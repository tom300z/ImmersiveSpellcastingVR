#include "RE/Skyrim.h"
#include "SKSE/SKSE.h"
#include "utils.h"
#include <ConfigManager.h>
#include <Settings.h>
#include <windows.h>
#include <Settings.h>
#include "compat/HapticSkyrimVR.h"


namespace Utils
{
    void ExecuteConsoleCommand(std::string_view command)
    {
        auto* scriptFactory = RE::IFormFactory::GetConcreteFormFactoryByType<RE::Script>();
        auto script = scriptFactory->Create();
        if (!script) {
            SKSE::log::warn("Failed to create Script object.");
            return;
        }

        script->SetCommand(command.data());
        script->CompileAndRun(nullptr);
        script->~Script();  // Free object memory manually
        SKSE::log::info("Executed script command: {}", command);
    }

	bool InGame()
	{
		auto* ui = RE::UI::GetSingleton();
		if (!ui || ui->GameIsPaused())
			return false;

		static constexpr std::string_view blockedMenus[] = {
			"Console", "InventoryMenu", "MagicMenu", "ContainerMenu", "BarterMenu",
			"Dialogue Menu", "MapMenu", "Journal Menu"
		};
		for (auto m : blockedMenus) {
			if (ui->IsMenuOpen(m))
				return false;
		}
		return true;
	}

	bool InvertVRInputForSpell(RE::SpellItem *spell) {
		bool invertInput = spell->GetCastingType() == RE::MagicSystem::CastingType::kConcentration;

		if (invertInput && (std::strcmp(spell->GetName(), "Telekinesis") == 0)) {
			// Treat telekinesis like a non-concentration spell input-wise
			invertInput = false;
		}
		return invertInput;
	}

	using MessageBoxResultCallbackFunc = std::function<void(unsigned int)>;

    class MessageBoxResultCallback final : public RE::IMessageBoxCallback
	{
		MessageBoxResultCallbackFunc _callback;

	public:
		~MessageBoxResultCallback() override = default;
		explicit MessageBoxResultCallback(const MessageBoxResultCallbackFunc& callback) :
			_callback(callback) {}
		void Run(Message message) override
		{
			_callback(static_cast<unsigned int>(message));
		}
	};

	void ShowMessageBox(const std::string& bodyText, const std::vector<std::string>& buttonTextValues, MessageBoxResultCallbackFunc callback)
	{
		const auto* factoryManager = RE::MessageDataFactoryManager::GetSingleton();
		const auto* uiStringHolder = RE::InterfaceStrings::GetSingleton();
		auto* factory = factoryManager->GetCreator<RE::MessageBoxData>(
			uiStringHolder->messageBoxData);  // "MessageBoxData" <--- can we just use this string?
		auto* messagebox = factory->Create();
		const RE::BSTSmartPointer<RE::IMessageBoxCallback> messageCallback = RE::make_smart<MessageBoxResultCallback>(callback);
		messagebox->callback = messageCallback;
		messagebox->bodyText = bodyText;
		for (auto& text : buttonTextValues) messagebox->buttonText.push_back(text.c_str());
		messagebox->QueueMessage();
	}

	namespace Input
	{
		// Returns a canonical string name for a given OpenVR controller key code
		const char* GetOpenVRButtonName(std::uint32_t keyCode, vr::ETrackedControllerRole side)
		{
			// see also RE::BSOpenVRControllerDevice::
			using Keys = RE::BSOpenVRControllerDevice::Keys;
			switch (keyCode) {
			case vr::EVRButtonId::k_EButton_SteamVR_Trigger:
				return "Trigger";
			case vr::EVRButtonId::k_EButton_Grip:
				return "Grip";
			case vr::EVRButtonId::k_EButton_Axis2:  // kGripAlt
				return "Grip Axis/Alt";
			case vr::EVRButtonId::k_EButton_SteamVR_Touchpad:
				return "Joystick Click";
			case vr::EVRButtonId::k_EButton_Axis3:  // kTouchpadClick
				return "Touchpad Click";
			case vr::EVRButtonId::k_EButton_Axis4:  // kTouchpadAlt
				return "Touchpad Alt";
			case vr::EVRButtonId::k_EButton_A:
			{
				switch (side) {
				case vr::ETrackedControllerRole::TrackedControllerRole_LeftHand:
					return "A";
				default:
					return "A/X";
				}
			}
			case vr::EVRButtonId::k_EButton_ApplicationMenu:
			{
				switch (side) {
				case vr::ETrackedControllerRole::TrackedControllerRole_LeftHand:
					return "B";
				default:
					return "B/Y";
				}
			}
			// Add any other explicit codes here if needed
			default:
				return "unknown";
			}
		}

		RE::BSTArray<RE::ControlMap::UserEventMapping> GetActiveVRUserEventMapping([[maybe_unused]] RE::UserEvents::INPUT_CONTEXT_ID context, bool leftHand)
		{
			//Get BSOpenVR
			auto bsOvr = RE::BSOpenVR::GetSingleton();

			// The HMD Type seems to actually be the controller type in OpenVR (Though i dont have multiple HMDs / Controllers to test that)
			auto hmdType = bsOvr->GetHMDDeviceType();
			auto currentRightdeviceMappingID = (RE::INPUT_DEVICES::INPUT_DEVICE)(RE::INPUT_DEVICES::kVivePrimary + (2 * (int)hmdType));
			auto currentLeftdeviceMappingID = (RE::INPUT_DEVICES::INPUT_DEVICE)(currentRightdeviceMappingID + 1);

			//logger::info("Current deviceMapping IDs (L/R): {}/{}", (int)currentLeftdeviceMappingID, (int)currentRightdeviceMappingID);

			// Get controlMap for gameplay
			auto controlMap = RE::ControlMap::GetSingleton();
			auto inputContext = controlMap->controlMap[RE::UserEvents::INPUT_CONTEXT_ID::kGameplay];

			return inputContext->deviceMappings[leftHand ? currentLeftdeviceMappingID : currentRightdeviceMappingID];
		}

		bool IsUsingIndexControllers()
		{
			auto leftHandIndex = vr::VRSystem()->GetTrackedDeviceIndexForControllerRole(vr::TrackedControllerRole_LeftHand);

			char buffer[200];
			vr::ETrackedPropertyError error = vr::TrackedProp_Success;
			const auto needed = vr::VRSystem()->GetStringTrackedDeviceProperty(
				leftHandIndex, vr::Prop_RenderModelName_String, buffer, sizeof(buffer), &error);
			logger::info("Left Hand Render model: {}", buffer);
			std::string renderModelName(buffer);

			return renderModelName.find("indexcontroller") != std::string::npos;
		}
	}

	namespace Setup
	{

		static void ShowBindingMessage(std::string unwantedMappings)
		{
			auto message = std::format(
				R"(WARNING: {} buttons used by Skyrim! Actions will be unavailable while spells are equipped:{}

You have 3 options to mitigate this:
a) Unmap the buttons in gameplay using the "VR Key Remapping Tool" from NexusMods.
b) Use the "Grip Touch" input instead of "Grip Press". See also HIGGS's "useTouchForGrip" (Index Controllers).
c) Ignore this warning and do without the actions. )",
				g_pluginName,
				unwantedMappings);
			std::vector<std::string> options = { "a) Show Remapping Tool on Nexusmods", "b) Use Touch input", "c) Ignore disabled actions", "Ignore once" };
			Utils::MessageBoxResultCallbackFunc handler;
			handler = [unwantedMappings](int index) {
				bool showAgain = false;
				if (index == 0) {
					ShellExecuteW(nullptr, L"open", L"https://www.nexusmods.com/skyrimspecialedition/mods/68164", nullptr, nullptr, SW_SHOWNORMAL);
					showAgain = true;
				} else if (index == 1) {
					Config::Manager::GetSingleton().SetValue("CastingInputMethod", "grip_touch");
				} else if (index == 2) {
					Config::Manager::GetSingleton().SetValue("ShowBindingWarning", false);
				}

				if (showAgain) {
					ShowBindingMessage(unwantedMappings);
				}
			};

			Utils::ShowMessageBox(
				message,
				options,
				handler);
		}

		// Checks for unwanted Keybindings and displays a warning message
		void CheckForUnwantedBindings()
		{
			if (!Config::Manager::GetSingleton().Get<bool>(Settings::kInputShowBindingWarning, true)) {
				return;
			}

			// Get userEventMappings for current left controller gameplay
			// Left grip key (can be changed at runtime)
			// RE::ControlMap::GetSingleton()->controlMap[RE::UserEvents::INPUT_CONTEXT_ID::kGameplay]->deviceMappings[6]

			auto unwantedMappings = ""s;
			for (bool leftHand : { true, false }) {
				auto userEventMappings = Utils::Input::GetActiveVRUserEventMapping(RE::UserEvents::INPUT_CONTEXT_ID::kGameplay, leftHand);
				auto sideName = leftHand ? "Left" : "Right";
				auto sideRole = leftHand ? vr::ETrackedControllerRole::TrackedControllerRole_LeftHand : vr::ETrackedControllerRole::TrackedControllerRole_RightHand;
				//logger::info("VR Gameplay controls ({}):", sideName);

				for (RE::ControlMap::UserEventMapping mapping : userEventMappings) {
					auto inputKeyName = Utils::Input::GetOpenVRButtonName(mapping.inputKey, sideRole);
					//logger::info("{} -> {} ({})", mapping.eventID.c_str(), inputKeyName, mapping.inputKey);

					// Show warning if grip is used in Gameplay and grip press is configured as input
					auto inputType = Config::Manager::GetSingleton().Get<std::string>(Settings::kInputMethod, "grip");
					if (inputType == "grip" && mapping.inputKey == vr::EVRButtonId::k_EButton_Grip) {
						unwantedMappings += std::format("\n {} {} Press -> {}", sideName, inputKeyName, mapping.eventID.c_str());
					}
				}
			}

			if (unwantedMappings.empty()) {
				return;
			}

			ShowBindingMessage(unwantedMappings);
		}

		void ShowOutdatedHSVRMsg() {
			Utils::ShowMessageBox(
				std::format(R"(WARNING: {} has detected that it is installed along an outdated version of Shizof's "Haptic Skyrim VR". This will lead to broken Spellcasting Haptics.

Please update Haptic Skyrim VR to version 1.8.0 or higher! )",
					g_pluginName),
				{ "Show Haptic Skyrim VR on Nexusmods", "Ignore once" },
				[](int index) {
					if (index == 0) {
						ShellExecuteW(nullptr, L"open", L"https://www.nexusmods.com/skyrimspecialedition/mods/20364", nullptr, nullptr, SW_SHOWNORMAL);
						ShowOutdatedHSVRMsg();
					}
				}
			);
		}

		void PerformInteractiveSetup() {
			CheckForUnwantedBindings();
			if (Compat::HapticSkyrimVR::hapticskyrimvrOutdated) {
				ShowOutdatedHSVRMsg();
			}
		}

		void SetupLogger() {
			// Set up logger
			/*
#ifndef NDEBUG
			auto sink = std::make_shared<spdlog::sinks::sink>();
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
			*/
		}
	}

	namespace Animation
	{

		void DumpPlayerGraphVariables()
		{
			auto* player = RE::PlayerCharacter::GetSingleton();
			if (!player) {
				return;
			}

			RE::BSTSmartPointer<RE::BSAnimationGraphManager> manager;
			if (!player->GetAnimationGraphManager(manager) || !manager) {
				logger::warn("No animation graph manager yet");
				return;
			}

			//RE::BSSpinLockGuard lock(manager->variableCache.updateLock);
			const auto& cache = manager->variableCache.variableCache;
			logger::info("Graph cache has {} entries", cache.size());

			std::uint32_t index = 0;
			for (auto& graphPtr : manager->graphs) {
				auto* graph = graphPtr.get();
				if (!graph) {
					continue;
				}

				auto* behavior = graph->behaviorGraph;
				auto* data = behavior ? behavior->data.get() : nullptr;
				auto* strings = data ? data->stringData.get() : nullptr;
				logger::info("Graph[{}]: {} has {} variables", index++, graph->projectName.c_str(), strings->variableNames.size());
				if (!strings) {
					continue;
				}

				for (const auto& hkName : strings->variableNames) {
					const auto name = hkName.c_str();
					float f;
					std::int32_t i;
					bool b;

					if (graph->GetGraphVariableFloat(name, f)) {
						logger::info("  {} = {} (float)", name, f);
					} else if (graph->GetGraphVariableInt(name, i)) {
						logger::info("  {} = {} (int)", name, i);
					} else if (graph->GetGraphVariableBool(name, b)) {
						logger::info("  {} = {} (bool)", name, b);
					}
				}
			}
		}
	}
}

namespace LogUtils
{
	constexpr std::string_view ToString(RE::MagicSystem::CastingType a_type)
	{
		using RE::MagicSystem::CastingType;
		switch (a_type) {
		case CastingType::kConstantEffect:
			return "Constant Effect";
		case CastingType::kFireAndForget:
			return "Fire and Forget";
		case CastingType::kConcentration:
			return "Concentration";
		case CastingType::kScroll:
			return "Scroll";
		default:
			return "Unknown";
		}
	}

	void LogHandItem(const std::string_view a_handLabel, bool a_isLeftHand, RE::PlayerCharacter* a_player)
	{
		bool gameIsLeftHand = a_isLeftHand;
		if (a_player && a_player->isLeftHandMainHand) {
			gameIsLeftHand = !a_isLeftHand;
		}
		if (auto* form = a_player->GetEquippedObject(gameIsLeftHand)) {
			logger::info("{} Hand: {} ({:08X})", a_handLabel, form->GetName(), form->GetFormID());

			if (auto* spell = form->As<RE::SpellItem>()) {
				//auto caster = a_player->GetMagicCaster(gameIsLeftHand ? MagicSystem::CastingSource::kLeftHand : MagicSystem::CastingSource::kRightHand);
				logger::info("  Spell Casting Type: {}", ToString(spell->GetCastingType()));
				logger::info("  Spell Charge Time: {:.2f}s", spell->GetChargeTime());
				/* DMMF::SetChargeTime(
					caster,
					2.0f
				);*/
			}
		} else {
			logger::info("{} Hand: empty", a_handLabel);
		}
	}

	// --- Our callback: log equipped item in each hand ---
	void LogEquippedItems()
	{
		auto* player = RE::PlayerCharacter::GetSingleton();
		if (!player) {
			logger::info("Player not found.");
			return;
		}

		LogHandItem("Left", true, player);
		LogHandItem("Right", false, player);
		return;
	}

	void LogBytes(const void* ptr, int number)
	{
		const uint8_t* p = static_cast<const uint8_t*>(ptr);
		std::string out;
		out.reserve(3 * number);  // "XX " × 8

		for (int i = 0; i < number; ++i)
			out += std::format("{:02X} ", p[i]);

		spdlog::info("Next {} bytes: {}", number, out);
	}
}

