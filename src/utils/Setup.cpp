#include "utils/Setup.h"

#include <format>
#include <shellapi.h>
#include <string>

#include "ConfigManager.h"
#include "Settings.h"
#include "compat/HIGGS.h"
#include "compat/HapticSkyrimVR.h"
#include "utils/Input.h"
#include "utils/MessageBox.h"
#include "utils/GameState.h"

#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/msvc_sink.h>

namespace Utils::Setup
{
	namespace
	{
		void ShowBindingMessage(const std::string& unwantedMappings)
		{
			auto message = std::format(
				R"(WARNING: {} buttons used by Skyrim! Actions will be unavailable while spells are equipped:{}

You have 3 options to mitigate this:
a) Unmap the buttons in gameplay using the "VR Key Remapping Tool" from NexusMods.
b) Use the "grip_touch" input instead of "grip_press". See also HIGGS's "useTouchForGrip" (Index Controllers).
c) Ignore this warning and do without the actions. )",
				g_pluginName,
				unwantedMappings);

			std::vector<std::string> options = {
				"a) Show Remapping Tool on Nexusmods",
				"b) Use grip_touch input",
				"c) Ignore disabled actions",
				"Ignore once"
			};

			MessageBoxResultCallbackFunc handler;
			handler = [unwantedMappings](int index) {
				bool showAgain = false;
				if (index == 0) {
					ShellExecuteW(nullptr, L"open", L"https://www.nexusmods.com/skyrimspecialedition/mods/68164", nullptr, nullptr, SW_SHOWNORMAL);
					showAgain = true;
				} else if (index == 1) {
					Config::Manager::GetSingleton().SetValue(Settings::kInputMethod, std::string{ "grip_touch" });
					if (Compat::HIGGSUseTouchForGrip::g_installed &&
						!std::get<bool>(Config::Manager::GetSingleton().GetValue(Settings::kInputHackHiggsTouchInput))) {
						ShowMessageBox(
							std::format(
								R"({} has noticed that you are using "grip_touch" as the input method and have HIGGS VR installed.

By default HIGGS uses "grip_press" for grabbing objects. {} can patch HIGGS to use "grip_touch" instead (can be toggled in MCM and ini).
If you are currently emulating "grip_press" via a SteamVR Binding, you can revert this change and use "grip_press" as a separate input from "grip_touch" again.)",
								g_pluginName, g_pluginNameShort, g_pluginNameShort),
							{ "Use grip_touch for HIGGS", "Keep using grip_press for HIGGS" },
							[](int result) {
								if (result == 0) {
									Config::Manager::GetSingleton().SetValue(Settings::kInputHackHiggsTouchInput, true);
								}
							});
					}
				} else if (index == 2) {
					Config::Manager::GetSingleton().SetValue(Settings::kInputShowBindingWarning, false);
				}

				if (showAgain) {
					ShowBindingMessage(unwantedMappings);
				}
			};

			ShowMessageBox(message, options, handler);
		}

		void CheckForUnwantedBindings()
		{
			if (!Config::Manager::GetSingleton().Get<bool>(Settings::kInputShowBindingWarning, true)) {
				return;
			}

			auto unwantedMappings = std::string{};
			for (bool leftHand : { true, false }) {
				auto userEvents = Utils::Input::GetActiveVRUserEventMapping(RE::UserEvents::INPUT_CONTEXT_ID::kGameplay, leftHand);
				auto sideName = leftHand ? "Left" : "Right";
				auto sideRole = leftHand ? vr::TrackedControllerRole_LeftHand : vr::TrackedControllerRole_RightHand;

				for (const auto& mapping : userEvents) {
					auto inputName = Utils::Input::GetOpenVRButtonName(mapping.inputKey, sideRole);
					auto inputType = Config::Manager::GetSingleton().Get<std::string>(Settings::kInputMethod, "grip_press");
					if (inputType == "grip_press" && mapping.inputKey == vr::EVRButtonId::k_EButton_Grip) {
						unwantedMappings += std::format("\n {} {} Press -> {}", sideName, inputName, mapping.eventID.c_str());
					}
				}
			}

			if (!unwantedMappings.empty()) {
				ShowBindingMessage(unwantedMappings);
			}
		}

		void ShowOutdatedHSVRMsg()
		{
			ShowMessageBox(
				std::format(R"(WARNING: {} has detected that it is installed along an outdated version of Shizof's "Haptic Skyrim VR". This will lead to broken Spellcasting Haptics.

Please update Haptic Skyrim VR to version 1.8.0 or higher! )",
					g_pluginName),
				{ "Show Haptic Skyrim VR on Nexusmods", "Ignore once" },
				[](int index) {
					if (index == 0) {
						ShellExecuteW(nullptr, L"open", L"https://www.nexusmods.com/skyrimspecialedition/mods/20364", nullptr, nullptr, SW_SHOWNORMAL);
						ShowOutdatedHSVRMsg();
					}
				});
		}
	}  // namespace

	void PerformInteractiveSetup()
	{
		CheckForUnwantedBindings();
		if (Compat::HapticSkyrimVR::hapticskyrimvrOutdated) {
			ShowOutdatedHSVRMsg();
		}
	}

	void SetupLogger()
	{
#ifndef NDEBUG
		auto sink = std::make_shared<spdlog::sinks::msvc_sink_mt>();
#else
		auto path = logger::log_directory();
		if (!path) {
			return;
		}

		*path /= g_pluginName;
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
	}
}
