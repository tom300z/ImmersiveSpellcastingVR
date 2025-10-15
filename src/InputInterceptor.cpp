#include "InputInterceptor.h"
#include "sksevr_api.h"
#include "utils.h"
#include "AttackState.h"

namespace InputInterceptor
{
	namespace
	{
		std::atomic_bool g_installed{ false };
	}

	static vr::EVRButtonId g_castingButtonId = vr::EVRButtonId::k_EButton_Grip;
	bool g_lastLeftcastingButtonState = false;
	bool g_lastRightcastingButtonState = false;

	void ControllerCallback(uint32_t unControllerDeviceIndex, vr::VRControllerState001_t* pControllerState, uint32_t unControllerStateSize, bool& state)
	{

		// Skip if this is not for a hand
		auto role = vr::VRSystem()->GetControllerRoleForTrackedDeviceIndex(unControllerDeviceIndex);
		if (!(role == vr::TrackedControllerRole_LeftHand || role == vr::TrackedControllerRole_RightHand)) {
			return;
		}
		const bool isLeftHand = role == vr::TrackedControllerRole_LeftHand;

		// Check if the casting button is pressed
		const bool castingButtonPressed = (pControllerState->ulButtonPressed & vr::ButtonMaskFromId(g_castingButtonId));

		// Skip if input did not change
		auto lastCastingButtonState = isLeftHand ? &g_lastLeftcastingButtonState : &g_lastRightcastingButtonState;
		if (*lastCastingButtonState == castingButtonPressed) {
			return;
		}
		*lastCastingButtonState = castingButtonPressed;


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

		// Dont hid a that causes the game to detect input after entering a menu if the cast button is currently held. Which sucks if the casting button is also the menu close button
		//// Hide the pressed casting button from the game to avoid unwanted input
		//pControllerState->ulButtonPressed &= ~vr::ButtonMaskFromId(g_castingButtonId);

		if (isConcentrationSpell) {
			// For concentration spells, the input is inverted, so that the spell is fired while the hand is open
			//AttackState::SetDesiredAttackingState(isLeftHand, !castingButtonPressed);
			AttackState::AddAttackButtonEvent(isLeftHand, !castingButtonPressed);
		} else {
			// For Fire&Forget spells & scrolls, pass the trigger state normally
			//AttackState::SetDesiredAttackingState(isLeftHand, castingButtonPressed);
			AttackState::AddAttackButtonEvent(isLeftHand, castingButtonPressed);
		}
	}

	void Install(const SKSE::LoadInterface* a_skse)
	{
		// Skip if already installed
		if (g_installed.exchange(true)) {
			return;
		}

		auto g_vrinterface = (SKSEVRInterface*)a_skse->QueryInterface(kInterface_VR);
		g_vrinterface->RegisterForControllerState(a_skse->GetPluginHandle(), 9999999999999, ControllerCallback);
		g_installed.store(false);
	}

}
