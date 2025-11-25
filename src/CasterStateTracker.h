#pragma once

#include "RE/Skyrim.h"

#include <atomic>
#include <cstdint>
#include <functional>
#include <functional>
#include "HandOrientation.h"

namespace CasterStateTracker
{
	enum class ActualState
	{
		kUnknown = -1,
		kIdle = 0,
		kStart,
		kCharging,
		kHolding,
		kReleasing,
		kChargingUnk,
		kCasting,
		kUnk07,
		kUnk08,
		kUnk09,
	};

	struct StateChangedEvent
	{
		HandOrientation::Info orientation;
		RE::MagicSystem::CastingSource castingSource;
		ActualState previousState{ ActualState::kUnknown };
		ActualState currentState{ ActualState::kUnknown };
		RE::ActorMagicCaster* caster;
	};

	using Listener = std::function<void(const StateChangedEvent&)>;

	extern std::atomic<ActualState> lastLeftHandState;
	extern std::atomic<ActualState> lastRightHandState;

	void Install();

	std::uint64_t AddListener(Listener listener);
	void RemoveListener(std::uint64_t id);
}
