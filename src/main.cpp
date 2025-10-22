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

void OnSaveLoadEvent([[maybe_unused]] RE::TESLoadGameEvent event)
{
	SpellChargeTracker::Install();
	Config::Checks::PostLoadCheck();
}

void OnMenuOpenCloseEvent([[maybe_unused]] RE::MenuOpenCloseEvent event)
{
	// Pause haptics while in menus
	Haptics::Pause(!utils::InGame());
}

void OnSKSEMessage(SKSE::MessagingInterface::Message* msg)
{
	switch (msg->type) {
	case SKSE::MessagingInterface::kPostPostLoad:
		{
			logger::info(FMT_STRING("kPostPostLoad"), Plugin::NAME, Plugin::VERSION);

			static auto menuHandler = utils::EventHandler<RE::MenuOpenCloseEvent>(OnMenuOpenCloseEvent);
			if (auto* ui = RE::UI::GetSingleton()) {
				ui->AddEventSink(&menuHandler);
			}
		}
		break;
	case SKSE::MessagingInterface::kDataLoaded:
		{
			logger::info(FMT_STRING("kDataLoaded"), Plugin::NAME, Plugin::VERSION);
			Config::Init();

			InputInterceptor::ConnectToConfig();
		}

		break;
	}
}


extern "C" DLLEXPORT bool SKSEAPI SKSEPlugin_Load(const SKSE::LoadInterface* a_skse)
{

	#ifdef _DEBUG
		// Wait for debugger
		while (!::IsDebuggerPresent());
	#endif

	SKSE::Init(a_skse);

	// SKSE::AllocTrampoline(1 << 10, false); // Unused for now, might come in handy later when i use write_call/write_branch

	auto messaging = SKSE::GetMessagingInterface();
	if (!messaging->RegisterListener("SKSE", OnSKSEMessage)) {
		logger::error("failed to init Message Handler");
		return false;
	}

	static auto loadGameHandler = utils::EventHandler<RE::TESLoadGameEvent>(OnSaveLoadEvent);
	auto* scriptEventSource = RE::ScriptEventSourceHolder::GetSingleton();
	if (scriptEventSource) {
		scriptEventSource->AddEventSink(
			&loadGameHandler
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

