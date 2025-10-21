#pragma once

#include <cstdint>
#include <filesystem>
#include <functional>
#include <optional>
#include <shared_mutex>
#include <string>
#include <string_view>
#include <unordered_map>
#include <variant>
#include <vector>

#include "RE/B/BSFixedString.h"

namespace Config
{
	enum class Type
	{
		kBool,
		kInteger,
		kFloat,
		kString
	};

	enum class ChangeSource
	{
		kFromIni,
		kFromMCM,
		kFromCode
	};

	using Value = std::variant<bool, std::int64_t, double, std::string>;

	struct Setting
	{
		Type type;
		Value value;
		Value defaultValue;
		std::string description;
	};

	class Manager
	{
	public:
		using Listener = std::function<void(std::string_view, const Value&, ChangeSource)>;

		[[nodiscard]] static Manager& GetSingleton();

		void RegisterSetting(std::string key, Type type, Value defaultValue, std::string description = {});

		void LoadFromDisk();
		void SaveToDisk();
		void ResetToDefaults();

		[[nodiscard]] bool HasKey(std::string_view key) const;
		[[nodiscard]] std::optional<Setting> GetSettingCopy(std::string_view key) const;
		[[nodiscard]] Value GetValue(std::string_view key) const;

		template <class T>
		[[nodiscard]] T Get(std::string_view key, T fallback) const;

		void SetValue(std::string_view key, Value value, ChangeSource source);

		std::uint64_t AddListener(Listener listener);
		void RemoveListener(std::uint64_t id);

		[[nodiscard]] std::filesystem::path GetIniPath() const;

		void DispatchFullSyncEvent(ChangeSource source);

	private:
		static Type ResolveValueType(const Value& value);
		static std::string SerializeValue(const Value& value, Type type);
		static std::optional<Value> DeserializeValue(Type type, std::string_view raw);
		static std::string_view TypeToString(Type type);

		void DispatchChangeEvent(std::string_view key, const Value& value, ChangeSource source);
		void EnsureIniPath() const;

		mutable std::shared_mutex _mutex;
		std::unordered_map<std::string, Setting> _settings;
		std::filesystem::path _iniPath;
		bool _dirty{ false };
		bool _loaded{ false };

		std::vector<std::pair<std::uint64_t, Listener>> _listeners;
		std::uint64_t _nextListenerId{ 1 };
	};

	bool RegisterPapyrusFunctions(RE::BSScript::IVirtualMachine* a_vm);

	namespace Checks
	{
		void PostLoadCheck();
	}
}

template <class T>
T Config::Manager::Get(std::string_view key, T fallback) const
{
	if (auto setting = GetSettingCopy(key)) {
		if (auto value = std::get_if<T>(&setting->value)) {
			return *value;
		}
	}
	return fallback;
}
