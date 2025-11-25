#pragma once

#include "RE/Skyrim.h"

namespace Utils
{
	static constexpr std::string_view kGameBlockingMenus[] = {
		"Console", "InventoryMenu", "MagicMenu", "ContainerMenu", "BarterMenu",
		"Dialogue Menu", "MapMenu", "Journal Menu", "TweenMenu"
	};

	bool InGame();
	bool IsPlayerHoldingSpell(bool mainHand);
	bool InvertVRInputForSpell(RE::SpellItem* spell);
}
