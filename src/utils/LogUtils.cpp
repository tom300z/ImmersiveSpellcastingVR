#include "utils/LogUtils.h"

#include <format>

#include "RE/Skyrim.h"

namespace LogUtils
{
	namespace
	{
		constexpr std::string_view ToString(RE::MagicSystem::CastingType type)
		{
			using RE::MagicSystem::CastingType;
			switch (type) {
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

		void LogHandItem(std::string_view handLabel, bool isLeftHand, RE::PlayerCharacter* player)
		{
			bool gameIsLeftHand = isLeftHand;
			if (player && player->isLeftHandMainHand) {
				gameIsLeftHand = !isLeftHand;
			}

			if (auto* form = player->GetEquippedObject(gameIsLeftHand)) {
				logger::info("{} Hand: {} ({:08X})", handLabel, form->GetName(), form->GetFormID());
				if (auto* spell = form->As<RE::SpellItem>()) {
					logger::info("  Spell Casting Type: {}", ToString(spell->GetCastingType()));
					logger::info("  Spell Charge Time: {:.2f}s", spell->GetChargeTime());
				}
			} else {
				logger::info("{} Hand: empty", handLabel);
			}
		}
	}

	void LogEquippedItems()
	{
		auto* player = RE::PlayerCharacter::GetSingleton();
		if (!player) {
			logger::info("Player not found.");
			return;
		}

		LogHandItem("Left", true, player);
		LogHandItem("Right", false, player);
	}

	void LogBytes(const void* ptr, int number)
	{
		const auto* data = static_cast<const std::uint8_t*>(ptr);
		std::string out;
		out.reserve(static_cast<std::size_t>(number) * 3);

		for (int i = 0; i < number; ++i) {
			out += std::format("{:02X} ", data[i]);
		}

		spdlog::info("Next {} bytes: {}", number, out);
	}
}
