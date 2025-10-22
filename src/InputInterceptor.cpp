#include "InputInterceptor.h"

#include <array>
#include <cctype>
#include <cstdint>
#include <limits>
#include <string>
#include <string_view>
#include <vector>

#include "AttackState.h"
#include "ConfigManager.h"
#include "Settings.h"
#include "sksevr_api.h"
#include "utils.h"
#include "haptics.h"
#include "math.h"

namespace InputInterceptor
{
	enum ButtonState
	{
		unknown,
		unpressed,
		pressed
	};

	vr::EVRButtonId g_castingButtonId = vr::EVRButtonId::k_EButton_Grip;
	bool g_castingButtonTouch = false;
	ButtonState g_lastLeftcastingButtonState = ButtonState::unknown;
	ButtonState g_lastRightcastingButtonState = ButtonState::unknown;

	namespace
	{
		std::atomic_bool g_installed{ false };
		std::uint64_t g_castingButtonListenerId{ 0 };

		struct ButtonMapping
		{
			std::string_view name;
			vr::EVRButtonId id;
		};

		void ApplyCastingInputMethod(const Config::Value& value)
		{
			std::string congfiguredInputMethod = *std::get_if<std::string>(&value);
			vr::EVRButtonId newButton = vr::EVRButtonId::k_EButton_Grip;
			bool newButtonTouch = false;
			if (congfiguredInputMethod == "grip") {
				newButton = vr::EVRButtonId::k_EButton_Grip;
				newButtonTouch = false;
			} else if (congfiguredInputMethod == "grip_touch") {
				newButton = vr::EVRButtonId::k_EButton_Grip;
				newButtonTouch = true;
			} else {
				logger::warn("Unsupported value type supplied for casting button configuration");
			}

			if (newButton != g_castingButtonId || newButtonTouch != g_castingButtonTouch) {

				g_castingButtonId = newButton;
				g_castingButtonTouch = newButtonTouch;

				g_lastLeftcastingButtonState = ButtonState::unknown;
				g_lastRightcastingButtonState = ButtonState::unknown;

				const auto name = utils::input::GetOpenVRButtonName(static_cast<std::uint32_t>(newButton));
				logger::info("Casting input method updated to {} {}", name ? name : "unknown", newButtonTouch ? "Touch" : "Grip");
			}
		}

		void ProcessCastingButtonState(bool isLeftHand, bool castingButtonActivated, bool forceDispatch)
		{
			const ButtonState newState = castingButtonActivated ? ButtonState::pressed : ButtonState::unpressed;
			ButtonState* lastCastingButtonState = isLeftHand ? &g_lastLeftcastingButtonState : &g_lastRightcastingButtonState;

			if (!forceDispatch) {
				if ((*lastCastingButtonState != ButtonState::unknown) && (*lastCastingButtonState == newState)) {
					return;
				}
			}

			*lastCastingButtonState = newState;

			auto player = RE::PlayerCharacter::GetSingleton();
			if (!(player && utils::InGame() && player->actorState2.weaponState == RE::WEAPON_STATE::kDrawn)) {
				return;
			}

			auto equippedObj = player->GetEquippedObject(isLeftHand);
			if (!equippedObj || !(equippedObj->GetFormType() == RE::FormType::Spell)) {
				return;
			}
			auto spell = static_cast<RE::SpellItem*>(equippedObj);

			const bool isConcentrationSpell = spell->GetCastingType() == RE::MagicSystem::CastingType::kConcentration;
			const bool desiredAttackPressed = isConcentrationSpell ? !castingButtonActivated : castingButtonActivated;

			AttackState::AddAttackButtonEvent(isLeftHand, desiredAttackPressed);
		}
	}

	void ControllerCallback(uint32_t unControllerDeviceIndex, vr::VRControllerState001_t* pControllerState, [[maybe_unused]] uint32_t unControllerStateSize, [[maybe_unused]] bool& state)
	{

		// Skip if this is not for a hand
		auto role = vr::VRSystem()->GetControllerRoleForTrackedDeviceIndex(unControllerDeviceIndex);
		if (!(role == vr::TrackedControllerRole_LeftHand || role == vr::TrackedControllerRole_RightHand)) {
			return;
		}
		const bool isLeftHand = role == vr::TrackedControllerRole_LeftHand;

		// Get the right mask for press or touch
		const uint64_t buttonActivationMask = g_castingButtonTouch ? pControllerState->ulButtonTouched : pControllerState->ulButtonPressed;

		// Check if the casting button is pressed/touched
		const bool castingButtonActivated = (buttonActivationMask & vr::ButtonMaskFromId(g_castingButtonId));

		ProcessCastingButtonState(isLeftHand, castingButtonActivated, false);

		// Dont hide a that causes the game to detect input after entering a menu if the cast button is currently held. Which sucks if the casting button is also the menu close button
		//// Hide the pressed casting button from the game to avoid unwanted input
		//pControllerState->ulButtonPressed &= ~vr::ButtonMaskFromId(g_castingButtonId);
	}

	// This needs to be called manually after the config was loaded
	void ConnectToConfig() {
		ApplyCastingInputMethod(Config::Manager::GetSingleton().GetValue(Settings::kCastingInputMethod));
		if (g_castingButtonListenerId == 0) {
			g_castingButtonListenerId = Config::Manager::GetSingleton().AddListener(
				[](std::string_view key, const Config::Value& value, [[maybe_unused]] Config::ChangeSource source) {
					if (key == Settings::kCastingInputMethod) {
						ApplyCastingInputMethod(value);
					}
				});
		}
	}

	void RefreshCastingState()
	{
		if (!utils::InGame()) {
			return;
		}

		if (g_lastLeftcastingButtonState != ButtonState::unknown) {
			const bool castingButtonActivated = g_lastLeftcastingButtonState == ButtonState::pressed;
			ProcessCastingButtonState(true, castingButtonActivated, true);
		}

		if (g_lastRightcastingButtonState != ButtonState::unknown) {
			const bool castingButtonActivated = g_lastRightcastingButtonState == ButtonState::pressed;
			ProcessCastingButtonState(false, castingButtonActivated, true);
		}
	}

	void Install(const SKSE::LoadInterface* a_skse)
	{
		// Skip if already installed
		if (g_installed.exchange(true)) {
			return;
		}

		auto g_vrinterface = (SKSEVRInterface*)a_skse->QueryInterface(kInterface_VR);
		g_vrinterface->RegisterForControllerState(a_skse->GetPluginHandle(), std::numeric_limits<std::int32_t>::max(), ControllerCallback);
		g_installed.store(false);
	}

}
