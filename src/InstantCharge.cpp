#include "InstantCharge.h"

#include "RE/Skyrim.h"
#include "spdlog/spdlog.h"

#include <cstdint>

namespace InstantCharge
{
	namespace
	{
		using SetCastingTimerForCharge_t = void(RE::ActorMagicCaster*);

		SetCastingTimerForCharge_t* g_original = nullptr;

		void Hook_SetCastingTimerForCharge(RE::ActorMagicCaster* a_caster)
		{
			if (g_original) {
				g_original(a_caster);
			}

			if (!a_caster) {
				return;
			}

			if (a_caster->castingTimer > 0.0f) {
				if (auto* spell = a_caster->currentSpell) {
					spdlog::debug("[InstantCharge] zero charge for {}", spell->GetName());
				}
				a_caster->castingTimer = 0.0f;
			}
		}
	}  // namespace

	void Install()
	{
		static bool installed = false;
		if (installed) {
			return;
		}

		REL::Relocation<std::uintptr_t> vtbl{ RE::VTABLE_ActorMagicCaster[0] };
		g_original = reinterpret_cast<SetCastingTimerForCharge_t*>(
			vtbl.write_vfunc(0x14, reinterpret_cast<std::uintptr_t>(&Hook_SetCastingTimerForCharge)));

		installed = true;
		spdlog::info("[InstantCharge] Hooked ActorMagicCaster::SetCastingTimerForCharge");
	}
}
