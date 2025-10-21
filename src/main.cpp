#include "spdlog/spdlog.h"

#include <atomic>
#include <chrono>
#include <format>
#include <string>
#include <string_view>
#include <thread>

#include "SKSE/API.h"
#include "SKSE/Interfaces.h"
#include "RE/Skyrim.h"
#include "SKSE/Trampoline.h"
#include "AttackState.h"
#include "ConfigManager.h"
#include "InputInterceptor.h"
#include "Settings.h"
#include "openvr.h"
#include "utils.h"
#include <windows.h>
#include <haptics.h>
#include "SpellChargeTracker.h"

using namespace RE;
using namespace SKSE;

extern "C" DLLEXPORT bool SKSEAPI SKSEPlugin_Query(const SKSE::QueryInterface* a_skse, SKSE::PluginInfo* a_info)
{
#ifndef NDEBUG
	auto sink = std::make_shared<spdlog::sinks::msvc_sink_mt>();
#else
	auto path = logger::log_directory();
	if (!path) {
		return false;
	}

	*path /= Plugin::NAME;
	*path += ".log"sv;
	auto sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>(path->string(), true);
#endif

	auto log = std::make_shared<spdlog::logger>("global log"s, std::move(sink));


#ifndef NDEBUG
	log->set_level(spdlog::level::trace);
#else
	log->set_level(spdlog::level::info);
	log->flush_on(spdlog::level::warn);
#endif

	spdlog::set_default_logger(std::move(log));
	spdlog::set_pattern("%s(%#): [%^%l%$] %v"s);

	logger::info(FMT_STRING("{} v{} queried"), Plugin::NAME, Plugin::VERSION);

	a_info->infoVersion = SKSE::PluginInfo::kVersion;
	a_info->name = Plugin::NAME.data();
	a_info->version = Plugin::VERSION.major();

	if (a_skse->IsEditor()) {
		logger::critical("Loaded in editor, marking as incompatible"sv);
		return false;
	}

	const auto ver = a_skse->RuntimeVersion();
	if (ver < SKSE::RUNTIME_VR_1_4_15) {
		logger::critical(FMT_STRING("Unsupported runtime version {}"), ver.string());
		return false;
	}

	return true;
}


void OnSKSEMessage(SKSE::MessagingInterface::Message* msg)
{
	switch (msg->type) {
	case SKSE::MessagingInterface::kInputLoaded:
		{
			logger::info(FMT_STRING("kInputLoaded"), Plugin::NAME, Plugin::VERSION);
			break;
		}
	case SKSE::MessagingInterface::kDataLoaded:
		{
			logger::info(FMT_STRING("kDataLoaded"), Plugin::NAME, Plugin::VERSION);
			break;
		}
	}
}

void OnSaveLoadEvent([[maybe_unused]] RE::TESLoadGameEvent event)
{
	SpellChargeTracker::Install();
	Config::Checks::PostLoadCheck();
}


extern "C" DLLEXPORT bool SKSEAPI SKSEPlugin_Load(const SKSE::LoadInterface* a_skse)
{

	#ifdef _DEBUG
		// Wait for debugger
		while (!::IsDebuggerPresent());
	#endif

	SKSE::Init(a_skse);

	auto& config = Config::Manager::GetSingleton();
	config.RegisterSetting(std::string(Settings::kCastingInputMethod), Config::Type::kString, Config::Value{ std::string("grip") }, "OpenVR button name that should be treated as the casting button.");
	config.RegisterSetting(std::string(Settings::kShowBindingWarning), Config::Type::kBool, Config::Value{ true }, "Show a warning when the grip button is bound in the gameplay context.");
	config.LoadFromDisk();
	config.SaveToDisk();

	// SKSE::AllocTrampoline(1 << 10, false); // Unused for now, might come in handy later when i use write_call/write_branch

	auto messaging = SKSE::GetMessagingInterface();
	if (!messaging->RegisterListener("SKSE", OnSKSEMessage)) {
		logger::error("failed to init Message Handler");
		return false;
	}

	static auto handler = utils::EventHandler<RE::TESLoadGameEvent>(OnSaveLoadEvent);
	auto* scriptEventSource = RE::ScriptEventSourceHolder::GetSingleton();
	if (scriptEventSource) {
		scriptEventSource->AddEventSink(
			&handler
		);
	}

	auto* papyrus = SKSE::GetPapyrusInterface();
	if (!papyrus->Register(Config::RegisterPapyrusFunctions)) {
		logger::error("Failed to register Papyrus interface for configuration");
		return false;
	}

	InputInterceptor::Install(a_skse);

	logger::info("{} v{} loaded"sv, Plugin::NAME, Plugin::VERSION.string());

	return true;
}

