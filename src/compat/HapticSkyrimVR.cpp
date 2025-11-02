#include "HapticSkyrimVR.h"

#include "REL/Relocation.h"
#include "REX/W32/KERNEL32.h"

#include <array>
#include <cstring>
#include <span>

namespace Compat::HapticSkyrimVR
{
	namespace
	{
		constexpr auto* kModuleName = "hapticskyrimvr_plugin.dll";

		constexpr std::array<std::uint8_t, 29> kSpellCastingPrologue{
			0x48, 0x89, 0x5C, 0x24, 0x10, 0x48, 0x89, 0x74, 0x24, 0x18,
			0x48, 0x89, 0x7C, 0x24, 0x20, 0x55, 0x41, 0x54, 0x41, 0x55,
			0x41, 0x56, 0x41, 0x57, 0x48, 0x8D, 0x6C, 0x24, 0xC9
		};

        constexpr std::array<std::uint8_t, 29> kReleaseBurstPrologue{
			0x48, 0x89, 0x5c, 0x24, 0x08, 0x48, 0x89, 0x74, 0x24, 0x18,
            0x48, 0x89, 0x7c, 0x24, 0x20, 0x55, 0x41, 0x54, 0x41, 0x55,
            0x41, 0x56, 0x41, 0x57, 0x48, 0x8d, 0x6c, 0x24, 0xc9
		};

		constexpr std::array<PatchTarget, 2> kTargetsTemplate{
            PatchTarget{
                .name = "SpellCastingEventDecider",
                .offset = 0x59B0,
                .prologue = kSpellCastingPrologue,
            },
            PatchTarget{
                .name = "ReleaseBurst",
                .offset = 0x7510,
                .prologue = kReleaseBurstPrologue,
            }
        };

		constexpr std::array<std::uint8_t, 5> kRetPatch{ 0xC3, 0x90, 0x90, 0x90, 0x90 };

		std::array<PatchTarget, kTargetsTemplate.size()> g_targets{ kTargetsTemplate };
		std::uintptr_t                                   g_moduleBase{ 0 };

		short resolve_targets()
		{
			if (g_moduleBase == 0) {
				const auto module = REX::W32::GetModuleHandleA(kModuleName);
				if (!module) {
					logger::debug("Compat::HapticSkyrimVR: module {} not loaded; skipping patch", kModuleName);
					return g_targets.size();
				}

				g_moduleBase = reinterpret_cast<std::uintptr_t>(module);
			}

			std::size_t mismatches = 0;
			for (auto& target : g_targets) {
				if (target.address != 0) {
					continue;
				}

				const auto address = g_moduleBase + target.offset;
				const auto* prologue = reinterpret_cast<const std::uint8_t*>(address);

				if (std::memcmp(prologue, target.prologue.data(), target.prologue.size()) != 0) {
					logger::warn("Compat::HapticSkyrimVR: prologue mismatch for {}; skipping patch for that function", target.name);
					++mismatches;
					continue;
				}

				target.address = address;
			}

			return mismatches;
		}
	}

	short DisableMagicHaptics(bool disabled)
	{
		const auto mismatches = resolve_targets();

		for (auto& target : g_targets) {
			if (target.address == 0) {
				continue;
			}

			if (disabled) {
				if (!target.saved) {
					std::memcpy(
						target.original.data(),
						reinterpret_cast<const void*>(target.address),
						target.original.size());
					target.saved = true;
				}

				REL::safe_write(
					target.address,
					kRetPatch.data(),
					kRetPatch.size());

			} else if (target.saved) {
				REL::safe_write(
					target.address,
					target.original.data(),
					target.original.size());
			}
		}

		return mismatches;
	}
}
