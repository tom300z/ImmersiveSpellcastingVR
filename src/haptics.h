#pragma once
#include <openvr.h>

namespace haptics
{
    class Manager
    {
    public:
        static void Initialize();
        static void Update(float leftCharge, float rightCharge);
        static void Shutdown();

    private:
        static inline vr::VRActionHandle_t hapticAction = vr::k_ulInvalidActionHandle;
        static inline vr::VRInputValueHandle_t leftHand = vr::k_ulInvalidInputValueHandle;
        static inline vr::VRInputValueHandle_t rightHand = vr::k_ulInvalidInputValueHandle;
        static inline bool initialized = false;
    };
}
