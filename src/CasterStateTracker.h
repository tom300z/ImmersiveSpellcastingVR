#pragma once

#include "RE/Skyrim.h"

#include <atomic>
#include <cstdint>
#include <functional>

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
		bool isPhysicalLeft{ false };
		bool isMainHand{ false };
		RE::MagicSystem::CastingSource castingSource{ RE::MagicSystem::CastingSource::kOther };
		ActualState previousState{ ActualState::kUnknown };
		ActualState currentState{ ActualState::kUnknown };
		RE::ActorMagicCaster* caster{ nullptr };
	};

	using Listener = std::function<void(const StateChangedEvent&)>;

	extern std::atomic<ActualState> lastLeftHandState;
	extern std::atomic<ActualState> lastRightHandState;

	void Install();

	std::uint64_t AddListener(Listener listener);
	void RemoveListener(std::uint64_t id);
}
