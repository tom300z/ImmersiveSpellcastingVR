#include "RE/Skyrim.h"
#include "SKSE/SKSE.h"

namespace util
{
    void ExecuteConsoleCommand(std::string_view command)
    {
        auto* scriptFactory = RE::IFormFactory::GetConcreteFormFactoryByType<RE::Script>();
        auto script = scriptFactory->Create();
        if (!script) {
            SKSE::log::warn("Failed to create Script object.");
            return;
        }

        script->SetCommand(command.data());
        script->CompileAndRun(nullptr);
        script->~Script();  // Free object memory manually
        SKSE::log::info("Executed script command: {}", command);
    }
}

namespace log_utils
{
	constexpr std::string_view ToString(RE::MagicSystem::CastingType a_type)
	{
		using RE::MagicSystem::CastingType;
		switch (a_type) {
		case CastingType::kConstantEffect:
			return "Constant Effect";
		case CastingType::kFireAndForget:
			return "Fire and Forget";
		case CastingType::kConcentration:
			return "Concentration";
		case CastingType::kScroll:
			return "Scroll";
		default:
			return "Unknown";
		}
	}

	void LogHandItem(const std::string_view a_handLabel, bool a_isLeftHand, RE::PlayerCharacter* a_player)
	{
		if (auto* form = a_player->GetEquippedObject(a_isLeftHand)) {
			logger::info("{} Hand: {} ({:08X})", a_handLabel, form->GetName(), form->GetFormID());

			if (auto* spell = form->As<RE::SpellItem>()) {
				//auto caster = a_player->GetMagicCaster(a_isLeftHand ? MagicSystem::CastingSource::kLeftHand : MagicSystem::CastingSource::kRightHand);
				logger::info("  Spell Casting Type: {}", ToString(spell->GetCastingType()));
				logger::info("  Spell Charge Time: {:.2f}s", spell->GetChargeTime());
				/* DMMF::SetChargeTime(
					caster,
					2.0f
				);*/
			}
		} else {
			logger::info("{} Hand: empty", a_handLabel);
		}
	}

	// --- Our callback: log equipped item in each hand ---
	void LogEquippedItems()
	{
		auto* player = RE::PlayerCharacter::GetSingleton();
		if (!player) {
			logger::info("Player not found.");
			return;
		}

		LogHandItem("Left", true, player);
		LogHandItem("Right", false, player);
		return;
	}

}
