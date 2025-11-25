#include "CasterStateTracker.h"

#include "REL/Relocation.h"
#include "SKSE/SKSE.h"
#include "HandOrientation.h"
#include "hooks/ActorMagicCaster.h"

#include <algorithm>
#include <mutex>
#include <vector>

namespace CasterStateTracker
{
	std::atomic<ActualState> lastLeftHandState = ActualState::kUnknown;
	std::atomic<ActualState> lastRightHandState = ActualState::kUnknown;

	namespace
	{
		using UpdateFunc = void (*)(RE::ActorMagicCaster*, float);

		std::atomic_bool g_installed{ false };
		UpdateFunc g_originalUpdate{ nullptr };

		std::mutex g_listenerMutex;
		std::vector<std::pair<std::uint64_t, Listener>> g_listeners;
		std::atomic<std::uint64_t> g_nextListenerId{ 1 };

		void DispatchEvent(const StateChangedEvent& event)
		{
			std::vector<Listener> snapshot;
			{
				std::scoped_lock lock(g_listenerMutex);
				for (const auto& [_, listener] : g_listeners) {
					if (listener) {
						snapshot.push_back(listener);
					}
				}
			}

			for (const auto& listener : snapshot) {
				listener(event);
			}
		}

		bool IsHandCaster(RE::ActorMagicCaster* caster)
		{
			switch (caster->castingSource) {
			case RE::MagicSystem::CastingSource::kLeftHand:
			case RE::MagicSystem::CastingSource::kRightHand:
				return true;
			default:
				return false;
			}
		}

		void UpdateState(RE::ActorMagicCaster* caster)
		{
			auto* player = RE::PlayerCharacter::GetSingleton();
			if (!player || caster->actor != player || !IsHandCaster(caster)) {
				return;
			}

			const auto orientation = HandOrientation::FromCastingSource(caster->castingSource);

			// When dual casting, use the left-hand caster state for both hands to avoid divergent timers.
			auto* stateSource = caster;
			if (player->IsDualCasting()) {
				if (auto* leftCaster = player->GetMagicCaster(RE::MagicSystem::CastingSource::kLeftHand)) {
					stateSource = reinterpret_cast<RE::ActorMagicCaster*>(leftCaster);
				}
			}

			const ActualState newState = static_cast<ActualState>(stateSource->state.get());
			auto& handState = orientation.isPhysicalLeft ? lastLeftHandState : lastRightHandState;
			const ActualState previousState = handState.exchange(newState, std::memory_order_relaxed);

			// Dispatch event if state changed or every frame while charging
			if (previousState == newState && newState != ActualState::kCharging) {
				return;
			}

			DispatchEvent({
				.orientation = orientation,
				.castingSource = caster->castingSource,
				.previousState = previousState,
				.currentState = newState,
				.caster = stateSource,
			});
		}

		void UpdateHook(RE::ActorMagicCaster* caster, float delta)
		{
			g_originalUpdate(caster, delta);

			UpdateState(caster);
		}
	}  // namespace

	std::uint64_t AddListener(Listener listener)
	{
		const auto id = g_nextListenerId.fetch_add(1, std::memory_order_relaxed);
		std::scoped_lock lock(g_listenerMutex);
		g_listeners.emplace_back(id, std::move(listener));
		return id;
	}

	void RemoveListener(std::uint64_t id)
	{
		std::scoped_lock lock(g_listenerMutex);
		const auto it = std::remove_if(g_listeners.begin(), g_listeners.end(), [id](const auto& entry) {
			return entry.first == id;
		});
		g_listeners.erase(it, g_listeners.end());
	}

	void Install()
	{
		if (g_installed.exchange(true)) {
			return;
		}

		constexpr std::size_t kUpdateIndex = 0x1F;
		if (!Hooks::ActorMagicCaster::WriteHook(kUpdateIndex, "CasterStateTracker", g_originalUpdate, &UpdateHook)) {
			g_installed.store(false);
			return;
		}
	}
}
