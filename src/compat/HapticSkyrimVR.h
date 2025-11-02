#pragma once

namespace Compat::HapticSkyrimVR {
    struct PatchTarget
    {
        const char*                                       name;
        std::uintptr_t                                    offset;
        std::span<const std::uint8_t>                     prologue;
        std::array<std::uint8_t, 5>                       original{};
        std::uintptr_t                                    address{ 0 };
        bool                                              saved{ false };
    };


	short DisableMagicHaptics(bool disabled);
}
