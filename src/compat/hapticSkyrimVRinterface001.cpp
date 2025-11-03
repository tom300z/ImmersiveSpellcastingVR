#include "hapticSkyrimVRinterface001.h"

// A message used to fetch hapticSkyrimVR's interface
struct HapticSkyrimVRMessage
{
	enum { kMessage_GetInterface = 0xFA66615D }; // Randomly chosen by the universe
	void* (*GetApiFunction)(unsigned int revisionNumber) = nullptr;
};

// Stores the API after it has already been fetched
HapticSkyrimVRPluginApi::IHapticSkyrimVRInterface001* g_hapticSkyrimVRInterface = nullptr;

// Fetches the interface to use from hapticSkyrimVR
HapticSkyrimVRPluginApi::IHapticSkyrimVRInterface001* HapticSkyrimVRPluginApi::getHapticSkyrimVRInterface001(const PluginHandle& pluginHandle, const SKSE::MessagingInterface* messagingInterface)
{
	// If the interface has already been fetched, return the same object
	if (g_hapticSkyrimVRInterface) {
		return g_hapticSkyrimVRInterface;
	}

	// Dispatch a message to get the plugin interface from hapticSkyrimVR
	HapticSkyrimVRMessage swMessage;
	if (messagingInterface->Dispatch(HapticSkyrimVRMessage::kMessage_GetInterface, (void*)&swMessage, sizeof(HapticSkyrimVRMessage*), "HapticSkyrimVRPlugin"))
	{
		logger::debug("hapticSkyrimVR dispatch message returned true...");
	}
	else
	{
		logger::debug("hapticSkyrimVR dispatch message returned false...");
	}

	if (!swMessage.GetApiFunction)
	{
		return nullptr;
	}

	// Fetch the API for this version of the Durability interface
	g_hapticSkyrimVRInterface = static_cast<HapticSkyrimVRPluginApi::IHapticSkyrimVRInterface001*>(swMessage.GetApiFunction(1));
	return g_hapticSkyrimVRInterface;
}
