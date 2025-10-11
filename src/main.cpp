#include "spdlog/spdlog.h"

#include <atomic>
#include <chrono>
#include <string_view>
#include <thread>

#include "SKSE/API.h"
#include "SKSE/Interfaces.h"
#include "RE/Skyrim.h"
#include "SKSE/SKSE.h"
#include "SKSE/API.h"
#include "SKSE/Trampoline.h"
#include "AttackTicker.h"
#include "openvr.h"
#include "util.h"
//#include "InstantCharge.h"
//#include "DMMF_API.h"

using namespace RE;
using namespace SKSE;

namespace
{
	std::atomic_bool g_palmLoggerRunning{ false };
	std::thread g_palmLoggerThread;

	std::string_view RoleToString(vr::ETrackedControllerRole role)
	{
		using vr::ETrackedControllerRole;
		switch (role) {
		case ETrackedControllerRole::TrackedControllerRole_LeftHand:
			return "left"sv;
		case ETrackedControllerRole::TrackedControllerRole_RightHand:
			return "right"sv;
		default:
			return "unknown"sv;
		}
	}

	void PalmButtonLoggerLoop()
	{
		using namespace std::chrono_literals;

		bool reportedMissingSystem = false;
		bool leftTouched = false;
		bool rightTouched = false;

		while (g_palmLoggerRunning.load(std::memory_order_acquire)) {
			auto* system = vr::VRSystem();
			if (!system) {
				if (!reportedMissingSystem) {
					logger::warn("OpenVR system not ready; waiting before polling palm button events."sv);
					reportedMissingSystem = true;
				}
				leftTouched = false;
				rightTouched = false;
				std::this_thread::sleep_for(500ms);
				continue;
			}

			reportedMissingSystem = false;

			const auto pollGripTouch = [&](vr::ETrackedControllerRole role, bool& wasTouched) {
				const vr::TrackedDeviceIndex_t index = system->GetTrackedDeviceIndexForControllerRole(role);
				if (index == vr::k_unTrackedDeviceIndexInvalid) {
					wasTouched = false;
					return;
				}

				if (system->GetTrackedDeviceClass(index) != vr::TrackedDeviceClass_Controller) {
					wasTouched = false;
					return;
				}

				vr::VRControllerState_t controllerState{};
				if (!system->GetControllerState(index, &controllerState, sizeof(controllerState))) {
					return;
				}

				const bool isTouched = (controllerState.ulButtonTouched & vr::ButtonMaskFromId(vr::k_EButton_Grip)) != 0;
				if (isTouched && !wasTouched) {
					logger::info("Palm button touched on {} controller (device {})", RoleToString(role), index);
				}
				wasTouched = isTouched;
			};

			pollGripTouch(vr::ETrackedControllerRole::TrackedControllerRole_LeftHand, leftTouched);
			pollGripTouch(vr::ETrackedControllerRole::TrackedControllerRole_RightHand, rightTouched);

			std::this_thread::sleep_for(10ms);  // Roughly 100 Hz polling to mirror VR framerate.
		}
	}

	void StartPalmButtonLogger()
	{
		if (g_palmLoggerRunning.exchange(true)) {
			return;
		}

		g_palmLoggerThread = std::thread(PalmButtonLoggerLoop);
		logger::info("Palm button logger started."sv);
	}

	void StopPalmButtonLogger()
	{
		if (!g_palmLoggerRunning.exchange(false)) {
			return;
		}

		if (g_palmLoggerThread.joinable()) {
			g_palmLoggerThread.join();
		}

		logger::info("Palm button logger stopped."sv);
	}

	struct PalmButtonLoggerGuard
	{
		~PalmButtonLoggerGuard() { StopPalmButtonLogger(); }
	} g_palmButtonLoggerGuard;
}


void OnSKSEMessage(SKSE::MessagingInterface::Message* msg)
{
	switch (msg->type) {
	case SKSE::MessagingInterface::kInputLoaded:
			{
				logger::info(FMT_STRING("kInputLoaded"), Plugin::NAME, Plugin::VERSION);
				StartPalmButtonLogger();
				break;
			}
		case SKSE::MessagingInterface::kPostLoadGame:
			{
				logger::info(FMT_STRING("kPostLoadGame"), Plugin::NAME, Plugin::VERSION);
				log_utils::LogEquippedItems();
				break;
			}
	}
}

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
	if (ver < SKSE::RUNTIME_SSE_1_4_2) {
		logger::critical(FMT_STRING("Unsupported runtime version {}"), ver.string());
		return false;
	}

	return true;
}

extern "C" DLLEXPORT bool SKSEAPI SKSEPlugin_Load(const SKSE::LoadInterface* a_skse)
{

	#ifdef _DEBUGG
		while (!::IsDebuggerPresent()) {
		}
	#endif

	SKSE::Init(a_skse);

    //AttackToggle::Start();
    //InstantCharge::Install();

	auto messaging = SKSE::GetMessagingInterface();

	if (!messaging->RegisterListener("SKSE", OnSKSEMessage)) {
		logger::error("failed to init Message Handler");
		return false;
	}

	auto* sourceHolder = ScriptEventSourceHolder::GetSingleton();
	if (sourceHolder) {
		sourceHolder->AddEventSink(log_utils::EquipEventHandler::GetSingleton());
		logger::info("Event handlers registered successfully.");
	}

	logger::info("{} v{} loaded"sv, Plugin::NAME, Plugin::VERSION.string());

	return true;
}

