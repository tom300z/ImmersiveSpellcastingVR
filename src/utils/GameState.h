#pragma once

#include "RE/Skyrim.h"

namespace Utils
{
	bool InGame();
	bool IsPlayerHoldingSpell(bool mainHand);
	bool InvertVRInputForSpell(RE::SpellItem* spell);
}
