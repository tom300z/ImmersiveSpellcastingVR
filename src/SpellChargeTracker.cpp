#include "SpellChargeTracker.h"

#include "REL/Relocation.h"
#include "SKSE/Logger.h"
#include "SKSE/SKSE.h"
#include "haptics.h"
#include "utils.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <optional>
#include <typeinfo>

namespace SpellChargeTracker
{
	bool installed = false;

	namespace
	{
		using UpdateFunc = void (*)(RE::ActorMagicCaster*, float);

		UpdateFunc g_originalUpdate{ nullptr };

		ActualState lastLeftHandState = ActualState::kUnknown;
		ActualState lastRightHandState = ActualState::kUnknown;

		void SetHapticState(RE::ActorMagicCaster* caster)
		{
			// Return if this is not the player 
			if (!caster || caster->actor != RE::PlayerCharacter::GetSingleton()) {
				return;
			}

			// Return if the magic source is not a hand
			if (!(caster->castingSource == RE::MagicSystem::CastingSource::kLeftHand || caster->castingSource == RE::MagicSystem::CastingSource::kRightHand)) {
				return;
			}
			const bool isLeftHand = caster->castingSource == RE::MagicSystem::CastingSource::kLeftHand;
			ActualState newState = (ActualState)caster->state.get();

			// Return if the state is unchanged and current state is idle/hold/release
			ActualState* lastStatePtr = (isLeftHand ? &lastLeftHandState : &lastRightHandState);
			if (
				(newState == *lastStatePtr) && 
				(newState == ActualState::kIdle || newState == ActualState::kHolding || newState == ActualState::kReleasing)
			) {
				return;
			}
			// Update last state
			*lastStatePtr = newState;

			// Update haptics
			auto handHaptics = Haptics::GetHandHaptics(isLeftHand);

			if (newState == ActualState::kIdle) {
				handHaptics->UpdateHapticState(0, 0, true);
				//logger::info("Setting haptics for idle...");
			} else if (newState == ActualState::kStart || newState == ActualState::kCharging) {
				float chargeProgress = (newState == ActualState::kCharging) ? (1 - (caster->castingTimer * 2)) : 0;  // castingTimer goes down from 0.5 to 0 (No idea why...)

				handHaptics->UpdateHapticState(
					std::lerp(100, 20, chargeProgress),
					std::pow(chargeProgress, 6),     // Using pow makes the interpolation exponential instead of linear
					newState == ActualState::kStart  // Interrupt current pulse on first update
				);
				//logger::info("Setting haptics for Progress: {}", chargeProgress);
			} else if (newState == ActualState::kHolding) {
				handHaptics->UpdateHapticState(50, 0.01, true);  // Stop haptics
			} else if (newState == ActualState::kReleasing) {
				handHaptics->UpdateHapticState(30, 1, true);
				//logger::info("Setting haptics for release!");
			}

		}

		void UpdateHook(RE::ActorMagicCaster* caster, float delta)
		{
			if (!g_originalUpdate) {
				return;
			}

			g_originalUpdate(caster, delta);
			SetHapticState(caster);
		}
	}  // namespace

	void Install()
	{
		if (installed) {
			return;
		}

		if (auto* player = RE::PlayerCharacter::GetSingleton()) {
			RE::ActorMagicCaster* sample = nullptr;
			for (auto slot : { RE::Actor::SlotTypes::kLeftHand, RE::Actor::SlotTypes::kRightHand }) {
				sample = player->magicCasters[slot];
				if (sample) {
					break;
				}
			}

			if (!sample) {
				logger::warn("SpellChargeTracker: player has no ActorMagicCaster instances yet");
				return;
			}

			auto** vtblPtr = reinterpret_cast<std::uintptr_t**>(sample);
			if (!vtblPtr || !*vtblPtr) {
				logger::warn("SpellChargeTracker: unable to resolve caster vtable");
				return;
			}

			constexpr std::size_t kUpdateIndex = 0x1F;
			auto* vtbl = *vtblPtr;

			g_originalUpdate = reinterpret_cast<UpdateFunc>(vtbl[kUpdateIndex]);
			const auto hookAddr = reinterpret_cast<std::uintptr_t>(&UpdateHook);
			REL::safe_write(reinterpret_cast<std::uintptr_t>(&vtbl[kUpdateIndex]), hookAddr);

			if (!g_originalUpdate) {
				logger::warn("SpellChargeTracker: original ActorMagicCaster::Update resolved as nullptr");
			}

			logger::info("SpellChargeTracker: original ActorMagicCaster::Update {:p}", reinterpret_cast<const void*>(g_originalUpdate));
			logger::info("SpellChargeTracker: vtable slot patched -> {:p}", reinterpret_cast<const void*>(vtbl[kUpdateIndex]));

			installed = true;
		} else {
			logger::warn("SpellChargeTracker: PlayerCharacter not available, skip hook");
		}
	}
}
