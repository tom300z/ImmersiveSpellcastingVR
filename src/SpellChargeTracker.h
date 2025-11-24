#pragma once

#include "CasterStateTracker.h"

namespace SpellChargeTracker
{
	using ActualState = CasterStateTracker::ActualState;

	inline std::atomic<ActualState>& lastLeftHandState = CasterStateTracker::lastLeftHandState;
	inline std::atomic<ActualState>& lastRightHandState = CasterStateTracker::lastRightHandState;

	/// Install spell haptics subscriber.
	void Install();
}
