#include "InputInterceptor.h"
#include "sksevr_api.h"

namespace InputInterceptor
{
	namespace
	{
		std::atomic_bool g_installed{ false };
	}
	
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
		auto role = vr::VRSystem()->GetControllerRoleForTrackedDeviceIndex(unControllerDeviceIndex);
		std::string side;
		switch (role) {
		case vr::TrackedControllerRole_LeftHand:
			side = "Left";
			break;
		case vr::TrackedControllerRole_RightHand:
			side = "Right";
			break;
		default:
			side = "Unknown";
			break;
		}
		for (uint32_t id = 0; id < vr::k_EButton_Max; ++id) {
			auto buttonId = static_cast<vr::EVRButtonId>(id);
			uint64_t mask = vr::ButtonMaskFromId(buttonId);
			if (pControllerState->ulButtonPressed & mask) {
				const char* name = GetOpenVRButtonName(id);
				logger::info("Pressed: {} {} (id={})", side, name ? name : "Unknown", id);

				if (buttonId == vr::EVRButtonId::k_EButton_SteamVR_Trigger) {
					pControllerState->ulButtonPressed &= ~vr::ButtonMaskFromId(buttonId);
					logger::info("Unpressed: {} {} (id={})", side, name ? name : "Unknown", id);
				}
			}
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
