#pragma once

#include "RE/Skyrim.h"
#include "openvr.h"

namespace Utils::Input
{
	const char* GetOpenVRButtonName(std::uint32_t keyCode, vr::ETrackedControllerRole side = vr::TrackedControllerRole_Invalid);
	RE::BSTArray<RE::ControlMap::UserEventMapping> GetActiveVRUserEventMapping(RE::UserEvents::INPUT_CONTEXT_ID context, bool leftHand);
	bool IsUsingIndexControllers();
}
