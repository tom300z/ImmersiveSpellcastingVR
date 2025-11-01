#pragma once

#include "RE/Skyrim.h"

namespace SpellChargeTracker
{
	enum class ActualState  // RE::MagicCaster::State seems to be annotated incorrectly
	{
		kUnknown = -1,
		kIdle = 0,
		kStart,     // Start?
		kCharging,  // StartCharge?
		kHolding,
		kReleasing,  // PreStart?
		kChargingUnk,
		kCasting,
		kUnk07,  // Unknown
		kUnk08,  // Interrupt
		kUnk09,  // Interrupt/Deselect
	};

	/// Install the detour for magic caster updates.
	void Install();
}
