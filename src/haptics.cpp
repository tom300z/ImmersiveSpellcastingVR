#include "haptics.h"
#include <RE/Skyrim.h>
#include <chrono>
#include <thread>
#include <algorithm>

using namespace std::chrono;
using namespace haptics;

void Manager::Initialize()
{
    if (initialized)
        return;

    vr::EVRInitError error = vr::VRInitError_None;
    vr::IVRSystem* vr_system = vr::VR_Init(&error, vr::VRApplication_Scene);
    if (error != vr::VRInitError_None) {
        _MESSAGE("haptics: Failed to initialize OpenVR: %s", vr::VR_GetVRInitErrorAsEnglishDescription(error));
        return;
    }

    // Set manifest (OpenVR 1.0.10 still uses the same call)
    const char* manifestPath = "Data\\SKSE\\Plugins\\actions.json";
    vr::VRInput()->SetActionManifestPath(manifestPath);

    // Get handles
    vr::VRInput()->GetActionHandle("/actions/main/out/haptic", &hapticAction);
    vr::VRInput()->GetInputSourceHandle("/user/hand/left", &leftHand);
    vr::VRInput()->GetInputSourceHandle("/user/hand/right", &rightHand);

    initialized = true;
    _MESSAGE("haptics: Initialized successfully.");
}

void Manager::Update(float leftCharge, float rightCharge)
{
    if (!initialized)
        return;

    static auto lastTime = steady_clock::now();
    auto now = steady_clock::now();
    float delta = duration<float>(now - lastTime).count();
    if (delta < 0.05f)  // 50 ms between pulses
        return;
    lastTime = now;

    auto triggerHaptic = [](vr::VRInputValueHandle_t hand, float charge) {
        if (charge <= 0.0f)
            return;

        float amplitude = std::clamp(charge, 0.0f, 1.0f);
        float frequency = 100.0f + 50.0f * amplitude;
        float duration = 0.05f;

        // OpenVR 1.0.10 uses the same call signature
        vr::VRInput()->TriggerHapticVibrationAction(
            hapticAction,
            0.0f,
            duration,
            frequency,
            amplitude,
            hand
        );
    };

    triggerHaptic(leftHand, leftCharge);
    triggerHaptic(rightHand, rightCharge);
}

void Manager::Shutdown()
{
    if (!initialized)
        return;

    vr::VR_Shutdown();
    initialized = false;
    _MESSAGE("haptics: Shutdown complete.");
}
