#include "RE/Skyrim.h"
#include "SKSE/SKSE.h"
#include "utils.h"
#include <ConfigManager.h>
#include <Settings.h>
#include <windows.h>
#include <Settings.h>

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
				return std::to_string(keyCode).c_str();
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
		// Checks for unwanted Keybindings and displays a warning message
		void CheckForUnwantedBindings()
		{
			if (!Config::Manager::GetSingleton().Get<bool>(Settings::kShowBindingWarning, true)) {
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
					auto inputType = Config::Manager::GetSingleton().Get<std::string>(Settings::kCastingInputMethod, "grip");
					if (inputType == "grip" && mapping.inputKey == vr::EVRButtonId::k_EButton_Grip) {
						unwantedMappings += std::format("\n {} {} Press -> {}", sideName, inputKeyName, mapping.eventID.c_str());
					}
				}
			}

			if (unwantedMappings.empty()) {
				return;
			}

			auto message = std::format(
				R"({} WARNING: buttons alrady mapped in the gameplay context!
{}

You have two options to mitigate this:

a. Use the "Grip Touch" input instead of "Grip Press" (See MCM). See also HIGGS's "useTouchForGrip" setting.
b. Unmap the buttons using the "VR Key Remapping Tool" from NexusMods)",
				Plugin::NAME,
				unwantedMappings);
			std::vector<std::string> options = { "Show Remapping Tool on Nexus", "Ignore once" };
			Utils::MessageBoxResultCallbackFunc handler;
			handler = [message, options](int index) {
				if (options[index] == "Show Remapping Tool on Nexus") {
					ShellExecuteW(nullptr, L"open", L"https://www.nexusmods.com/skyrimspecialedition/mods/68164", nullptr, nullptr, SW_SHOWNORMAL);
					Utils::ShowMessageBox(message, { "Ignore once" });
				}
			};

			Utils::ShowMessageBox(
				message,
				options,
				handler);
		}
	}
}

namespace log_utils
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
		if (auto* form = a_player->GetEquippedObject(a_isLeftHand)) {
			logger::info("{} Hand: {} ({:08X})", a_handLabel, form->GetName(), form->GetFormID());

			if (auto* spell = form->As<RE::SpellItem>()) {
				//auto caster = a_player->GetMagicCaster(a_isLeftHand ? MagicSystem::CastingSource::kLeftHand : MagicSystem::CastingSource::kRightHand);
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
}

