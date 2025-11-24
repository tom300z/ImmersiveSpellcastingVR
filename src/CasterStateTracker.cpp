#include "CasterStateTracker.h"

#include "REL/Relocation.h"
#include "SKSE/SKSE.h"

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
			if (!caster) {
				return false;
			}

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
			if (!caster) {
				return;
			}

			auto* player = RE::PlayerCharacter::GetSingleton();
			if (!player || caster->actor != player || !IsHandCaster(caster)) {
				return;
			}

			const bool isMainHand = caster->castingSource == RE::MagicSystem::CastingSource::kRightHand;
			const bool isPhysicalLeft = RE::BSOpenVRControllerDevice::IsLeftHandedMode() ? isMainHand : !isMainHand;

			// When dual casting, use the left-hand caster state for both hands to avoid divergent timers.
			auto* stateSource = caster;
			if (player->IsDualCasting()) {
				if (auto* leftCaster = player->GetMagicCaster(RE::MagicSystem::CastingSource::kLeftHand)) {
					stateSource = reinterpret_cast<RE::ActorMagicCaster*>(leftCaster);
				}
			}

			if (!stateSource) {
				return;
			}

			const ActualState newState = static_cast<ActualState>(stateSource->state.get());
			auto& handState = isPhysicalLeft ? lastLeftHandState : lastRightHandState;
			const ActualState previousState = handState.exchange(newState, std::memory_order_relaxed);

			if (previousState == newState) {
				return;
			}

			DispatchEvent({
				.isPhysicalLeft = isPhysicalLeft,
				.isMainHand = isMainHand,
				.castingSource = caster->castingSource,
				.previousState = previousState,
				.currentState = newState,
				.caster = stateSource,
			});
		}

		void UpdateHook(RE::ActorMagicCaster* caster, float delta)
		{
			if (g_originalUpdate) {
				g_originalUpdate(caster, delta);
			}

			UpdateState(caster);
		}
	}  // namespace

	std::uint64_t AddListener(Listener listener)
	{
		if (!listener) {
			return 0;
		}

		const auto id = g_nextListenerId.fetch_add(1, std::memory_order_relaxed);
		std::scoped_lock lock(g_listenerMutex);
		g_listeners.emplace_back(id, std::move(listener));
		return id;
	}

	void RemoveListener(std::uint64_t id)
	{
		if (id == 0) {
			return;
		}

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

		auto* player = RE::PlayerCharacter::GetSingleton();
		if (!player) {
			logger::warn("CasterStateTracker: PlayerCharacter not available, skip hook");
			g_installed.store(false);
			return;
		}

		RE::ActorMagicCaster* sample = nullptr;
		for (auto slot : { RE::Actor::SlotTypes::kLeftHand, RE::Actor::SlotTypes::kRightHand }) {
			sample = player->magicCasters[slot];
			if (sample) {
				break;
			}
		}

		if (!sample) {
			logger::warn("CasterStateTracker: player has no ActorMagicCaster instances yet");
			g_installed.store(false);
			return;
		}

		auto** vtblPtr = reinterpret_cast<std::uintptr_t**>(sample);
		if (!vtblPtr || !*vtblPtr) {
			logger::warn("CasterStateTracker: unable to resolve caster vtable");
			g_installed.store(false);
			return;
		}

		constexpr std::size_t kUpdateIndex = 0x1F;
		auto* vtbl = *vtblPtr;

		g_originalUpdate = reinterpret_cast<UpdateFunc>(vtbl[kUpdateIndex]);
		const auto hookAddr = reinterpret_cast<std::uintptr_t>(&UpdateHook);
		REL::safe_write(reinterpret_cast<std::uintptr_t>(&vtbl[kUpdateIndex]), hookAddr);

		if (!g_originalUpdate) {
			logger::warn("CasterStateTracker: original ActorMagicCaster::Update resolved as nullptr");
		}

		logger::info("CasterStateTracker: original ActorMagicCaster::Update {:p}", reinterpret_cast<const void*>(g_originalUpdate));
		logger::info("CasterStateTracker: vtable slot patched -> {:p}", reinterpret_cast<const void*>(vtbl[kUpdateIndex]));
	}
}
