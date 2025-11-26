#include "ConfigManager.h"

#include <algorithm>
#include <cctype>
#include <iomanip>
#include <locale>
#include <map>
#include <sstream>
#include <vector>
#include <string>

#include "REL/Relocation.h"
#include "SKSE/API.h"
#include "utils/Input.h"
#include <utils.h>
#include <Settings.h>
#include <SimpleIni.h>

namespace
{
	using namespace std::literals;

	std::string_view TrimView(std::string_view input)
	{
		const auto first = input.find_first_not_of(" \t\r\n"sv);
		if (first == std::string_view::npos) {
			return {};
		}
		const auto last = input.find_last_not_of(" \t\r\n"sv);
		return input.substr(first, last - first + 1);
	}

	std::string TrimCopy(std::string_view input)
	{
		const auto trimmed = TrimView(input);
		return { trimmed.begin(), trimmed.end() };
	}

	std::string ToLower(std::string_view input)
	{
		std::string result;
		result.reserve(input.size());
		for (const auto ch : input) {
			result.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(ch))));
		}
		return result;
	}

	constexpr auto kModEventName = "ImmersiveCastingVR_ConfigChanged"sv;
}

namespace Config
{

	Manager& Manager::GetSingleton()
	{
		static Manager singleton;
		return singleton;
	}

	void Manager::RegisterSetting(std::string key, Type type, Value defaultValue, std::string description, std::string section)
	{
		if (ResolveValueType(defaultValue) != type) {
			logger::warn("Config setting '{}' default value type does not match the provided type; coercing", key);
			if (auto coerced = DeserializeValue(type, SerializeValue(defaultValue, ResolveValueType(defaultValue)))) {
				defaultValue = *coerced;
			} else {
				logger::error("Failed to coerce default value for '{}'; using fallback", key);
				switch (type) {
				case Type::kBool:
					defaultValue = false;
					break;
				case Type::kInteger:
					defaultValue = static_cast<std::int64_t>(0);
					break;
				case Type::kFloat:
					defaultValue = 0.0;
					break;
				case Type::kString:
					defaultValue = std::string{};
					break;
				}
			}
		}

		std::unique_lock lock(_mutex);
		Setting setting{};
		setting.type = type;
		setting.value = defaultValue;
		setting.defaultValue = defaultValue;
		setting.description = description;
		setting.section = section;
		auto [it, inserted] = _settings.try_emplace(
			key,
			std::move(setting)
		);

		if (!inserted) {
			it->second.type = type;
			it->second.defaultValue = defaultValue;
			if (!_loaded) {
				it->second.value = defaultValue;
			}
			if (!description.empty()) {
				it->second.description = std::move(description);
			}
			if (!section.empty()) {
				it->second.section = std::move(section);
			}
		}
	}

	void Manager::LoadFromDisk()
	{
		EnsureIniPath();

		CSimpleIniA ini;
		ini.SetUnicode();

		if (std::filesystem::exists(_iniPath)) {
			if (const auto rc = ini.LoadFile(_iniPath.string().c_str()); rc < 0) {
				logger::warn("Failed to load config file '{}' (error {})", _iniPath.string(), rc);
			}
		}

		std::vector<std::pair<std::string, Value>> changedSettings;
		{
			std::unique_lock lock(_mutex);
			for (auto& [key, setting] : _settings) {
				Value newValue = setting.defaultValue;
				const auto section = setting.section.empty() ? "" : setting.section.c_str();
				if (const char* raw = ini.GetValue(section, key.c_str(), nullptr)) {
					if (auto parsed = DeserializeValue(setting.type, raw)) {
						newValue = *parsed;
					} else {
						logger::warn("Config key '{}' has invalid value '{}'; falling back to default", key, raw);
					}
				}

				if (!_loaded || setting.value != newValue) {
					setting.value = newValue;
					changedSettings.emplace_back(key, setting.value);
				}
			}
			_loaded = true;
		}

		for (const auto& [key, value] : changedSettings) {
			DispatchChangeEvent(key, value, ChangeSource::kFromIni);
		}
	}

	void Manager::SaveToDisk()
	{
		EnsureIniPath();

		std::map<std::string, std::vector<std::pair<std::string, Setting>>> sections;
		{
			std::shared_lock lock(_mutex);

			for (const auto& [key, setting] : _settings) {
				sections[setting.section].emplace_back(key, setting);
			}
		}

		for (auto& [_, settings] : sections) {
			std::sort(settings.begin(), settings.end(), [](const auto& lhs, const auto& rhs) {
				return lhs.first < rhs.first;
			});
		}

		CSimpleIniA ini;
		ini.SetUnicode();
		for (const auto& [section, settings] : sections) {
			const auto* sectionName = section.c_str();
			for (const auto& [key, setting] : settings) {
				const auto value = SerializeValue(setting.value, setting.type);
				std::string comment_storage;
				const char* comment = nullptr;
				if (!setting.description.empty()) {
					comment_storage = std::format("; {}", setting.description);
					comment = comment_storage.c_str();
				}
				ini.SetValue(sectionName, key.c_str(), value.c_str(), comment, true);
			}
		}

		std::error_code ec;
		std::filesystem::create_directories(_iniPath.parent_path(), ec);
		if (ec) {
			logger::error("Failed to create directories for '{}' ({})", _iniPath.string(), ec.message());
			return;
		}

		if (const auto rc = ini.SaveFile(_iniPath.string().c_str()); rc < 0) {
			logger::error("Failed to save config file '{}' (error {})", _iniPath.string(), rc);
			return;
		}

		{
			std::unique_lock lock(_mutex);
		}
	}

	void Manager::ResetToDefaults()
	{
		std::vector<std::pair<std::string, Value>> changed;

		{
			std::unique_lock lock(_mutex);
			for (auto& [key, setting] : _settings) {
				if (setting.value != setting.defaultValue) {
					setting.value = setting.defaultValue;
					changed.emplace_back(key, setting.value);
				}
			}
		}

		for (const auto& [key, value] : changed) {
			DispatchChangeEvent(key, value, ChangeSource::kFromCode);
		}
	}

	bool Manager::HasKey(std::string_view key) const
	{
		const std::string keyStr(key);
		std::shared_lock lock(_mutex);
		return _settings.contains(keyStr);
	}

	std::optional<Setting> Manager::GetSettingCopy(std::string_view key) const
	{
		const std::string keyStr(key);
		std::shared_lock lock(_mutex);
		if (const auto it = _settings.find(keyStr); it != _settings.end()) {
			return it->second;
		}
		return std::nullopt;
	}

	Value Manager::GetValue(std::string_view key) const
	{
		if (auto setting = GetSettingCopy(key)) {
			return setting->value;
		}
		logger::warn("Requested config key '{}' which is not registered", key);
		return Value{ false };
	}

	void Manager::SetValue(std::string_view key, Value value, ChangeSource source, bool saveFile)
	{
		const std::string keyStr(key);

		Value storedValue{};

		{
			std::unique_lock lock(_mutex);
			const auto it = _settings.find(keyStr);
			if (it == _settings.end()) {
				logger::warn("Attempted to set unregistered config key '{}'", key);
				return;
			}

			const auto expectedType = it->second.type;
			const auto incomingType = ResolveValueType(value);
			if (incomingType != expectedType) {
				if (auto coerced = DeserializeValue(expectedType, SerializeValue(value, incomingType))) {
					value = *coerced;
				} else {
					logger::warn("Failed to coerce value for '{}' to expected type; ignoring update", key);
					return;
				}
			}

			if (it->second.value == value) {
				return;
			}

			it->second.value = value;
			storedValue = it->second.value;
		}

		DispatchChangeEvent(keyStr, storedValue, source);
		if (saveFile) {
			Manager::GetSingleton().SaveToDisk();
		}
	}

	std::uint64_t Manager::AddListener(Listener listener)
	{
		if (!listener) {
			return 0;
		}

		std::unique_lock lock(_mutex);
		const auto id = _nextListenerId++;
		_listeners.emplace_back(id, std::move(listener));
		return id;
	}

	void Manager::RemoveListener(std::uint64_t id)
	{
		if (id == 0) {
			return;
		}

		std::unique_lock lock(_mutex);
		const auto it = std::remove_if(
			_listeners.begin(),
			_listeners.end(),
			[id](const auto& entry) { return entry.first == id; }
		);
		_listeners.erase(it, _listeners.end());
	}

	std::filesystem::path Manager::GetIniPath() const
	{
		EnsureIniPath();
		return _iniPath;
	}

	void Manager::DispatchFullSyncEvent(ChangeSource source)
	{
		std::vector<std::pair<std::string, Value>> snapshot;
		{
			std::shared_lock lock(_mutex);
			snapshot.reserve(_settings.size());
			for (const auto& [key, setting] : _settings) {
				snapshot.emplace_back(key, setting.value);
			}
		}

		for (const auto& [key, value] : snapshot) {
			DispatchChangeEvent(key, value, source);
		}
	}

	Config::Type Manager::ResolveValueType(const Value& value)
	{
		return std::visit(
			[](auto&& val) -> Type {
				using T = std::decay_t<decltype(val)>;
				if constexpr (std::is_same_v<T, bool>) {
					return Type::kBool;
				} else if constexpr (std::is_same_v<T, std::int64_t>) {
					return Type::kInteger;
				} else if constexpr (std::is_same_v<T, double>) {
					return Type::kFloat;
				} else if constexpr (std::is_same_v<T, std::string>) {
					return Type::kString;
				} else {
					static_assert(!sizeof(T), "Unsupported config value type");
				}
			},
			value
		);
	}

	std::string Manager::SerializeValue(const Value& value, Type type)
	{
		std::ostringstream stream;
		stream.imbue(std::locale::classic());

		switch (type) {
		case Type::kBool:
			stream << (std::get<bool>(value) ? "true" : "false");
			break;
		case Type::kInteger:
			stream << std::get<std::int64_t>(value);
			break;
		case Type::kFloat:
			stream << std::setprecision(6) << std::fixed << std::get<double>(value);
			break;
		case Type::kString:
			{
				const auto& str = std::get<std::string>(value);
				if (str.find_first_of(" \t") != std::string::npos) {
					stream << '"' << str << '"';
				} else {
					stream << str;
				}
				break;
			}
		}

		return stream.str();
	}

	std::optional<Value> Manager::DeserializeValue(Type type, std::string_view raw)
	{
		switch (type) {
		case Type::kBool:
			{
				const auto lower = ToLower(raw);
				if (lower == "true" || lower == "1" || lower == "yes" || lower == "on") {
					return Value{ true };
				}
				if (lower == "false" || lower == "0" || lower == "no" || lower == "off") {
					return Value{ false };
				}
				return std::nullopt;
			}
		case Type::kInteger:
			{
				try {
					const auto value = std::stoll(std::string(raw));
					return Value{ static_cast<std::int64_t>(value) };
				} catch (...) {
					return std::nullopt;
				}
			}
		case Type::kFloat:
			{
				try {
					const auto value = std::stod(std::string(raw));
					return Value{ static_cast<double>(value) };
				} catch (...) {
					return std::nullopt;
				}
			}
		case Type::kString:
			{
				auto str = TrimCopy(raw);
				if (!str.empty() && str.front() == '"' && str.back() == '"' && str.size() >= 2) {
					str = str.substr(1, str.size() - 2);
				}
				if (!str.empty()) {
					return Value{ str };
				}
				return std::nullopt;
			}
		}

		return std::nullopt;
	}

	std::string_view Manager::TypeToString(Type type)
	{
		switch (type) {
		case Type::kBool:
			return "bool"sv;
		case Type::kInteger:
			return "integer"sv;
		case Type::kFloat:
			return "float"sv;
		case Type::kString:
			return "string"sv;
		default:
			return "unknown"sv;
		}
	}

	void Manager::DispatchChangeEvent(std::string_view key, const Value& value, ChangeSource source)
	{
		std::vector<std::pair<std::uint64_t, Listener>> listenersSnapshot;
		{
			std::shared_lock lock(_mutex);
			listenersSnapshot = _listeners;
		}

		for (const auto& [_, callback] : listenersSnapshot) {
			if (callback) {
				callback(key, value, source);
			}
		}

		if (auto* eventSource = SKSE::GetModCallbackEventSource()) {
			static const RE::BSFixedString eventName(kModEventName.data());
			std::string keyStr(key);
			SKSE::ModCallbackEvent event;
			event.eventName = eventName;
			event.strArg = RE::BSFixedString(keyStr.c_str());
			event.numArg = static_cast<float>(static_cast<std::int32_t>(source));
			event.sender = nullptr;
			eventSource->SendEvent(&event);
		}

		logger::debug("Config '{}' updated (type: {})", key, TypeToString(ResolveValueType(value)));
	}

	void Manager::EnsureIniPath() const
	{
		if (!_iniPath.empty()) {
			return;
		}

		std::filesystem::path path = std::filesystem::current_path() / std::filesystem::path{ "data\\skse\\plugins" };

		path /= g_pluginNameShort;
		path.replace_extension(".ini");

		const_cast<Manager*>(this)->_iniPath = std::move(path);
	}

	namespace Papyrus
	{
		constexpr auto kScriptName = "ImmersiveCastingVR_Config"sv;

		bool GetBool(RE::StaticFunctionTag*, RE::BSFixedString key)
		{
			return Manager::GetSingleton().Get<bool>(key.c_str(), false);
		}

		std::int32_t GetInt(RE::StaticFunctionTag*, RE::BSFixedString key)
		{
			return static_cast<std::int32_t>(Manager::GetSingleton().Get<std::int64_t>(key.c_str(), 0));
		}

		float GetFloat(RE::StaticFunctionTag*, RE::BSFixedString key)
		{
			return static_cast<float>(Manager::GetSingleton().Get<double>(key.c_str(), 0.0));
		}

		RE::BSFixedString GetString(RE::StaticFunctionTag*, RE::BSFixedString key)
		{
			auto result = RE::BSFixedString(Manager::GetSingleton().Get<std::string>(key.c_str(), {}).c_str());
			return result;
		}

		void SetBool(RE::StaticFunctionTag*, RE::BSFixedString key, bool value)
		{
			Manager::GetSingleton().SetValue(key.c_str(), Value{ value }, ChangeSource::kFromMCM);
		}

		void SetInt(RE::StaticFunctionTag*, RE::BSFixedString key, std::int32_t value)
		{
			Manager::GetSingleton().SetValue(key.c_str(), Value{ static_cast<std::int64_t>(value) }, ChangeSource::kFromMCM);
		}

		void SetFloat(RE::StaticFunctionTag*, RE::BSFixedString key, float value)
		{
			Manager::GetSingleton().SetValue(key.c_str(), Value{ static_cast<double>(value) }, ChangeSource::kFromMCM);
		}

		void SetString(RE::StaticFunctionTag*, RE::BSFixedString key, RE::BSFixedString value)
		{
			Manager::GetSingleton().SetValue(key.c_str(), Value{ std::string(value.c_str()) }, ChangeSource::kFromMCM);
		}

		void Reload(RE::StaticFunctionTag*)
		{
			Manager::GetSingleton().LoadFromDisk();
			Manager::GetSingleton().DispatchFullSyncEvent(ChangeSource::kFromIni);
		}

		void Save(RE::StaticFunctionTag*)
		{
			Manager::GetSingleton().SaveToDisk();
		}

		void ResetAll(RE::StaticFunctionTag*)
		{
			Manager::GetSingleton().ResetToDefaults();
			Manager::GetSingleton().SaveToDisk();
		}
	}

	bool RegisterPapyrusFunctions(RE::BSScript::IVirtualMachine* a_vm)
	{
		using namespace Papyrus;

		if (!a_vm) {
			return false;
		}

		a_vm->RegisterFunction("GetBool", kScriptName.data(), GetBool);
		a_vm->RegisterFunction("GetInt", kScriptName.data(), GetInt);
		a_vm->RegisterFunction("GetFloat", kScriptName.data(), GetFloat);
		a_vm->RegisterFunction("GetString", kScriptName.data(), GetString);
		a_vm->RegisterFunction("SetBool", kScriptName.data(), SetBool);
		a_vm->RegisterFunction("SetInt", kScriptName.data(), SetInt);
		a_vm->RegisterFunction("SetFloat", kScriptName.data(), SetFloat);
		a_vm->RegisterFunction("SetString", kScriptName.data(), SetString);
		a_vm->RegisterFunction("Reload", kScriptName.data(), Reload);
		a_vm->RegisterFunction("Save", kScriptName.data(), Save);
		a_vm->RegisterFunction("ResetAll", kScriptName.data(), ResetAll);

		return true;
	}

	bool initialized = false;

	void Init()
	{
		if (initialized) {
			return;
		}

		auto& config = Manager::GetSingleton();
		for (const auto& def : Settings::GetSettingDefinitions()) {
			Value defaultValue = def.dynamicDefault ? def.dynamicDefault() : def.defaultValue;
			config.RegisterSetting(
				std::string(def.key),
				def.type,
				std::move(defaultValue),
				std::string(def.description),
				std::string(def.section));
		}

		config.LoadFromDisk();
		config.SaveToDisk();

		initialized = true;
	}
}
