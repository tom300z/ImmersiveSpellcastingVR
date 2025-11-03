#pragma once
#include "SKSE/API.h"
#include <string>

using namespace SKSE;

namespace HapticSkyrimVRPluginApi {
	// Returns an IHapticSkyrimVRInterface001 object compatible with the API shown below
	// This should only be called after SKSE sends kMessage_PostLoad to your plugin
	class IHapticSkyrimVRInterface001;
	IHapticSkyrimVRInterface001* getHapticSkyrimVRInterface001(const PluginHandle& pluginHandle, const SKSE::MessagingInterface* messagingInterface);

	// This object provides access to hapticSkyrimVR's mod support API
	class IHapticSkyrimVRInterface001
	{
	public:
		// Gets the build number
		virtual unsigned int getBuildNumber() = 0;

		virtual void ChangeSetting(std::string variableName, std::string variableValue) = 0;

		virtual void SpellcastingHapticsToggle(bool enableHaptics) = 0;

	};
}

extern HapticSkyrimVRPluginApi::IHapticSkyrimVRInterface001* g_hapticSkyrimVRInterface;
