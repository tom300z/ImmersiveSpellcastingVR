#pragma once

#include "RE/Skyrim.h"

namespace HandOrientation
{
    struct Info
    {
        bool isPhysicalLeft{ false };
        bool isMainHand{ false };

        RE::MagicSystem::CastingSource castingSource{ RE::MagicSystem::CastingSource::kOther };
        std::uint32_t slotIndex{ 0 };
    };

    inline Info FromPhysical(bool physicalLeft)
    {
        const bool isMain = RE::BSOpenVRControllerDevice::IsLeftHandedMode() ? physicalLeft : !physicalLeft;
        Info info{};
        info.isPhysicalLeft = physicalLeft;
        info.isMainHand = isMain;
        info.castingSource = isMain ? RE::MagicSystem::CastingSource::kRightHand : RE::MagicSystem::CastingSource::kLeftHand;
        info.slotIndex = isMain ? 1u : 0u;
        return info;
    }

    inline Info FromCastingSource(RE::MagicSystem::CastingSource source)
    {
        const bool isMain = (source == RE::MagicSystem::CastingSource::kRightHand);
        const bool physicalLeft = RE::BSOpenVRControllerDevice::IsLeftHandedMode() ? isMain : !isMain;
        Info info{};
        info.isPhysicalLeft = physicalLeft;
        info.isMainHand = isMain;
        info.castingSource = source;
        info.slotIndex = isMain ? 1u : 0u;
        return info;
    }
}
