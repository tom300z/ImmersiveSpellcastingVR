#include "spdlog/spdlog.h"

#include <atomic>
#include <chrono>
#include <string_view>
#include <thread>

#include "SKSE/API.h"
#include "SKSE/Interfaces.h"
#include "RE/Skyrim.h"
#include "SKSE/SKSE.h"
#include "SKSE/Trampoline.h"
#include "AttackState.h"
#include "InputInterceptor.h"
#include "openvr.h"
#include "utils.h"
#include <windows.h>

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
	case SKSE::MessagingInterface::kPostLoadGame:
		{
			logger::info(FMT_STRING("kPostLoadGame"), Plugin::NAME, Plugin::VERSION);
			break;
		}
	}
}

void OnSaveLoadEvent(RE::TESLoadGameEvent event)
{
	// Get userEventMappings for current left controller gameplay
	// Left grip key (can be changed at runtime)
	// RE::ControlMap::GetSingleton()->controlMap[RE::UserEvents::INPUT_CONTEXT_ID::kGameplay]->deviceMappings[6]

	auto unwantedMappings = ""s;
	for (bool leftHand : { true, false }) {
		auto userEventMappings = utils::input::GetActiveVRUserEventMapping(RE::UserEvents::INPUT_CONTEXT_ID::kGameplay, leftHand);
		auto sideName = leftHand ? "Left" : "Right";
		auto sideRole = leftHand ? vr::ETrackedControllerRole::TrackedControllerRole_LeftHand : vr::ETrackedControllerRole::TrackedControllerRole_RightHand;
		//logger::info("VR Gameplay controls ({}):", sideName);


		for (RE::ControlMap::UserEventMapping mapping : userEventMappings) {
			auto inputKeyName = utils::input::GetOpenVRButtonName(mapping.inputKey, sideRole);
			//logger::info("{} -> {} ({})", mapping.eventID.c_str(), inputKeyName, mapping.inputKey);

			// Show warning if grip is used in Gameplay
			if (mapping.inputKey == vr::EVRButtonId::k_EButton_Grip) {
				unwantedMappings += std::format("\n {} {} -> {}", sideName, inputKeyName, mapping.eventID.c_str());
			}
		}
	}

	auto message = std::format(
		R"(Warning, the following buttons used by {} are alrady bound in the gameplay context!
{}

Please unmap the key using the "Skyrim VR Change Your Bindings - Key Remapping Tool" from NexusMods)",
		Plugin::NAME,
		unwantedMappings);
	utils::ShowMessageBox(
		message,
		{ "Open NexusMods Page", "Ok" },
		[message](int index) {
			if (index == 0) {
				ShellExecuteW(nullptr, L"open", L"https://www.nexusmods.com/skyrimspecialedition/mods/68164", nullptr, nullptr, SW_SHOWNORMAL);
				utils::ShowMessageBox(message);
			}
		}
	);
}

extern "C" DLLEXPORT bool SKSEAPI SKSEPlugin_Load(const SKSE::LoadInterface* a_skse)
{

	#ifdef _DEBUG
		// Wait for debugger
		while (!::IsDebuggerPresent()) {
		}
	#endif

	SKSE::Init(a_skse);


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

	InputInterceptor::Install(a_skse);

	logger::info("{} v{} loaded"sv, Plugin::NAME, Plugin::VERSION.string());

	return true;
}

