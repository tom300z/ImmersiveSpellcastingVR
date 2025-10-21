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

		// Skip if input did not change
		ButtonState* lastCastingButtonState = isLeftHand ? &g_lastLeftcastingButtonState : &g_lastRightcastingButtonState;
		if (
			(*lastCastingButtonState != ButtonState::unknown) &&
			((*lastCastingButtonState == ButtonState::pressed) == castingButtonActivated)
			) {
			return;
		}
		*lastCastingButtonState = castingButtonActivated? ButtonState::pressed : ButtonState::unpressed;


		// Skip if player is not ready, in game and has weapons drawn
		auto player = RE::PlayerCharacter::GetSingleton();
		if (!(player && utils::InGame() && player->actorState2.weaponState == RE::WEAPON_STATE::kDrawn)) {
			return;
		}

		// Get equipped object, proceed if spell
		auto equippedObj = player->GetEquippedObject(isLeftHand);
		if (!(equippedObj->GetFormType() == RE::FormType::Spell)) {
			return;
		}
		auto spell = (RE::SpellItem*)equippedObj;

		// Get spell type
		bool isConcentrationSpell = spell->GetCastingType() == RE::MagicSystem::CastingType::kConcentration;

		// Dont hide a that causes the game to detect input after entering a menu if the cast button is currently held. Which sucks if the casting button is also the menu close button
		//// Hide the pressed casting button from the game to avoid unwanted input
		//pControllerState->ulButtonPressed &= ~vr::ButtonMaskFromId(g_castingButtonId);

		if (isConcentrationSpell) {
			// For concentration spells, the input is inverted, so that the spell is fired while the hand is open
			//AttackState::SetDesiredAttackingState(isLeftHand, !castingButtonPressed);
			AttackState::AddAttackButtonEvent(isLeftHand, !castingButtonActivated);
		} else {
			// For Fire&Forget spells & scrolls, pass the trigger state normally
			//AttackState::SetDesiredAttackingState(isLeftHand, castingButtonPressed);
			if (castingButtonActivated) {
				auto t = std::thread(Haptics::PlayHapticChargeEvent, Haptics::GetHandHaptics(isLeftHand), spell->GetChargeTime());
				t.detach();
			} else {
				Haptics::GetHandHaptics(isLeftHand)->UpdateHapticState(0, 0, false);
			}
			AttackState::AddAttackButtonEvent(isLeftHand, castingButtonActivated);
		}
	}

	void Install(const SKSE::LoadInterface* a_skse)
	{
		// Skip if already installed
		if (g_installed.exchange(true)) {
			return;
		}

		ApplyCastingInputMethod(Config::Manager::GetSingleton().GetValue(Settings::kCastingInputMethod));
		if (g_castingButtonListenerId == 0) {
			g_castingButtonListenerId = Config::Manager::GetSingleton().AddListener(
				[](std::string_view key, const Config::Value& value, [[maybe_unused]] Config::ChangeSource source) {
					if (key == Settings::kCastingInputMethod) {
						ApplyCastingInputMethod(value);
					}
				}
			);
		}

		auto g_vrinterface = (SKSEVRInterface*)a_skse->QueryInterface(kInterface_VR);
		g_vrinterface->RegisterForControllerState(a_skse->GetPluginHandle(), std::numeric_limits<std::int32_t>::max(), ControllerCallback);
		g_installed.store(false);
	}

}
