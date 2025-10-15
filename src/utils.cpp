#include "RE/Skyrim.h"
#include "SKSE/SKSE.h"

namespace utils
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

	void ShowMessageBox(const std::string& bodyText, const std::vector<std::string>& buttonTextValues = { "Ok" }, MessageBoxResultCallbackFunc callback = [](std::uint32_t) {})
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

	namespace input
	{
		// Returns a canonical string name for a given OpenVR controller key code
		const char* GetOpenVRButtonName(std::uint32_t keyCode, vr::ETrackedControllerRole side = vr::ETrackedControllerRole::TrackedControllerRole_Invalid)
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
				std::to_string(keyCode);
			}
		}

		RE::BSTArray<RE::ControlMap::UserEventMapping> GetActiveVRUserEventMapping(RE::UserEvents::INPUT_CONTEXT_ID context, bool leftHand)
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

