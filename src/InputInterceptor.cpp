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

	static vr::EVRButtonId g_castingButtonId = vr::EVRButtonId::k_EButton_Axis2;

	// Returns a canonical string name for a given OpenVR controller key code
	inline const char* GetOpenVRButtonName(std::uint32_t keyCode)
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
			return "A/X";
		case vr::EVRButtonId::k_EButton_ApplicationMenu:
			return "B/Y";
		// Add any other explicit codes here if needed
		default:
			std::to_string(keyCode);
		}
	}

	void ControllerCallback(uint32_t unControllerDeviceIndex, vr::VRControllerState001_t* pControllerState, uint32_t unControllerStateSize, bool& state)
	{
		// Skip if this is not for a hand
		auto role = vr::VRSystem()->GetControllerRoleForTrackedDeviceIndex(unControllerDeviceIndex);
		if (!(role == vr::TrackedControllerRole_LeftHand || role == vr::TrackedControllerRole_RightHand)) {
			return;
		}
		const bool isLeftHand = role == vr::TrackedControllerRole_LeftHand;

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

		// Check if the casting button is pressed
		const bool castingButtonPressed = (pControllerState->ulButtonPressed & vr::ButtonMaskFromId(g_castingButtonId));

		// Hide the pressed casting button from the game to avoid unwanted input
		pControllerState->ulButtonPressed &= ~vr::ButtonMaskFromId(g_castingButtonId);

		if (isConcentrationSpell) {
			// For concentration spells, the input is inverted, so that the spell is fired while the hand is open
			AttackState::SetDesiredAttackingState(isLeftHand, !castingButtonPressed);
		} else {
			// For Fire&Forget spells & scrolls, pass the trigger state normally
			AttackState::SetDesiredAttackingState(isLeftHand, castingButtonPressed);
		}
	}

	bool Install(const SKSE::LoadInterface* a_skse)
	{
		if (g_installed.exchange(true)) {
			return true;
		}

		auto g_vrinterface = (SKSEVRInterface*)a_skse->QueryInterface(kInterface_VR);
		g_vrinterface->RegisterForControllerState(a_skse->GetPluginHandle(), 9999999999999, ControllerCallback);
		g_installed.store(false);
	}

}
