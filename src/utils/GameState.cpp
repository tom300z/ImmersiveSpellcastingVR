#include "utils/GameState.h"

#include <cstring>

#include "RE/Skyrim.h"

namespace Utils
{
	bool InGame()
	{
		auto* ui = RE::UI::GetSingleton();
		if (!ui || ui->GameIsPaused()) {
			return false;
		}


		for (auto menu : kGameBlockingMenus) {
			if (ui->IsMenuOpen(menu)) {
				return false;
			}
		}

		return true;
	}

	bool IsPlayerHoldingSpell(bool mainHand)
	{
		const auto player = RE::PlayerCharacter::GetSingleton();
		return player &&
		       player->actorState2.weaponState == RE::WEAPON_STATE::kDrawn &&
		       player->GetEquippedObject(!mainHand) &&
		       player->GetEquippedObject(!mainHand)->GetFormType() == RE::FormType::Spell;
	}

	bool InvertVRInputForSpell(RE::SpellItem* spell)
	{
		if (!spell) {
			return false;
		}

		bool invertInput = spell->GetCastingType() == RE::MagicSystem::CastingType::kConcentration;
		if (invertInput && (std::strcmp(spell->GetName(), "Telekinesis") == 0)) {
			invertInput = false;
		}
		return invertInput;
	}
}
