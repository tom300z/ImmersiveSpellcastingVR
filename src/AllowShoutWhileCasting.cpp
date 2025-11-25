#include <utils/BinaryPatcher.h>
#include "hooks/ActorMagicCaster.h"

namespace AllowShoutWhileCasting
{
	/* std::array<Utils::BinaryPatcher::PatchTarget, 1> kTargets{
		Utils::BinaryPatcher::PatchTarget{
			.name = "allowShoutToInterruptCasting",
			.offset = 0x5468cf,
			.original{ 0x41, 0xB6, 0x01, 0xEB, 0x03, 0x45, 0x32, 0xF6, 0x41, 0xFF, 0xC5, 0x44, 0x3B, 0x6F, 0x68, 0x0F, 0x82, 0x10, 0xFF, 0xFF, 0xFF },
			.patched{ 0x41, 0xB6, 0x00 } }
	};

	Utils::BinaryPatcher g_patcher{
		"SkyrimVR.exe",
		"AllowShoutWhileCasting",
		std::span(kTargets)
	};*/

	namespace
	{
		using CheckCastFunc = bool (*)(RE::ActorMagicCaster*, RE::MagicItem*, bool, float*, RE::MagicSystem::CannotCastReason*, bool);

		CheckCastFunc g_originalCheckCast{ nullptr };
		bool g_installed{ false };

		bool CheckCastHook(RE::ActorMagicCaster* magicCaster, RE::MagicItem* spellOrShout, bool bDualCast, float* pfEffectiveStrength,
			RE::MagicSystem::CannotCastReason* pCannotCastReason, bool bUseBaseValueForCost)
		{
			bool result = g_originalCheckCast(magicCaster, spellOrShout, bDualCast, pfEffectiveStrength, pCannotCastReason, bUseBaseValueForCost);

			if (!result && magicCaster->actor == RE::PlayerCharacter::GetSingleton() && spellOrShout->GetSpellType() == RE::MagicSystem::SpellType::kVoicePower && *pCannotCastReason == RE::MagicSystem::CannotCastReason::kShoutWhileCasting) {
				// Interrupt hand casters and try again
				for (auto caster : { magicCaster->actor->GetMagicCaster(RE::MagicSystem::CastingSource::kLeftHand), magicCaster->actor->GetMagicCaster(RE::MagicSystem::CastingSource::kRightHand) }) {
					caster->InterruptCast(true);
				}
				result = g_originalCheckCast(magicCaster, spellOrShout, bDualCast, pfEffectiveStrength, pCannotCastReason, bUseBaseValueForCost);
			}

			// Re-run TEST AL, AL is unnecessary in C++ because returning a bool
			// already sets AL and flags correctly. Equivalent to: test al, al
			return result;
		}
	}  // namespace

	void Install()
	{
		if (g_installed) {
			return;
		}

		constexpr std::size_t kCheckCastIndex = 10;
		if (!Hooks::ActorMagicCaster::WriteHook(kCheckCastIndex, "AllowShoutWhileCasting", g_originalCheckCast, &CheckCastHook)) {
			return;
		}

		g_installed = true;
	}
}
