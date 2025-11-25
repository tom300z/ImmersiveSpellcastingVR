#include "utils/Input.h"

#include "RE/Skyrim.h"

namespace Utils::Input
{
	const char* GetOpenVRButtonName(std::uint32_t keyCode, vr::ETrackedControllerRole side)
	{
		switch (keyCode) {
		case vr::EVRButtonId::k_EButton_SteamVR_Trigger:
			return "Trigger";
		case vr::EVRButtonId::k_EButton_Grip:
			return "Grip";
		case vr::EVRButtonId::k_EButton_Axis2:
			return "Grip Axis/Alt";
		case vr::EVRButtonId::k_EButton_SteamVR_Touchpad:
			return "Joystick Click";
		case vr::EVRButtonId::k_EButton_Axis3:
			return "Touchpad Click";
		case vr::EVRButtonId::k_EButton_Axis4:
			return "Touchpad Alt";
		case vr::EVRButtonId::k_EButton_A:
			return side == vr::TrackedControllerRole_LeftHand ? "A" : "A/X";
		case vr::EVRButtonId::k_EButton_ApplicationMenu:
			return side == vr::TrackedControllerRole_LeftHand ? "B" : "B/Y";
		default:
			return "unknown";
		}
	}

	RE::BSTArray<RE::ControlMap::UserEventMapping> GetActiveVRUserEventMapping(RE::UserEvents::INPUT_CONTEXT_ID context, bool leftHand)
	{
		auto* bsOvr = RE::BSOpenVR::GetSingleton();
		auto* controlMap = RE::ControlMap::GetSingleton();
		if (!bsOvr || !controlMap) {
			return {};
		}

		auto hmdType = bsOvr->GetHMDDeviceType();
		const auto rightMapping = static_cast<RE::INPUT_DEVICES::INPUT_DEVICE>(RE::INPUT_DEVICES::kVivePrimary + (2 * static_cast<int>(hmdType)));
		const auto leftMapping = static_cast<RE::INPUT_DEVICES::INPUT_DEVICE>(rightMapping + 1);

		auto* inputContext = controlMap->controlMap[context];
		if (!inputContext) {
			return {};
		}

		return inputContext->deviceMappings[leftHand ? leftMapping : rightMapping];
	}

	bool IsUsingIndexControllers()
	{
		auto leftHandIndex = vr::VRSystem()->GetTrackedDeviceIndexForControllerRole(vr::TrackedControllerRole_LeftHand);

		char buffer[200] = {};
		vr::ETrackedPropertyError error = vr::TrackedProp_Success;
		vr::VRSystem()->GetStringTrackedDeviceProperty(leftHandIndex, vr::Prop_RenderModelName_String, buffer, sizeof(buffer), &error);
		if (error != vr::TrackedProp_Success) {
			return false;
		}

		return std::string_view(buffer).find("indexcontroller") != std::string_view::npos;
	}
}
