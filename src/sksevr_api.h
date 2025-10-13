#pragma once

#include <stdint.h>
#include <openvr.h>

enum
{
	kInterface_VR = 0x10,
};

struct SKSEVRInterface
{
	enum
	{
		kInterfaceVersion = 1,
		kOpenVRSourceVersion = 0x01000C00,
		kOpenVRTargetVersion = 0x01070F00
	};

	uint32_t interfaceVersion;
	uint32_t openvrSourceVersion;
	uint32_t openvrTargetVersion;

	typedef void(*ControllerStateCallback)(uint32_t unControllerDeviceIndex, vr::VRControllerState001_t *pControllerState, uint32_t unControllerStateSize, bool& state);
	typedef bool (*WaitGetPosesCallback)(vr::TrackedDevicePose_t* pRenderPoseArray, uint32_t unRenderPoseArrayCount, vr::TrackedDevicePose_t* pGamePoseArray, uint32_t unGamePoseArrayCount);

	bool(*IsActionsEnabled)();

	// Setting state to false will allow plugins to still execute their callbacks and optionally block, but at the very end will discard the input event
	void (*RegisterForControllerState)(SKSE::PluginHandle plugin, int32_t sortOrder, ControllerStateCallback callback);
	void (*RegisterForPoses)(SKSE::PluginHandle plugin, int32_t sortOrder, WaitGetPosesCallback callback);
};
