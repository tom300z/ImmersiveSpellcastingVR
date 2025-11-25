#include "hooks/ActorMagicCaster.h"

namespace Hooks::ActorMagicCaster
{
	namespace
	{
		std::uintptr_t* g_playerCasterVTable = nullptr;
	}

	std::uintptr_t* ResolveCasterVTable(std::string_view logTag)
	{
		if (g_playerCasterVTable) {
			return g_playerCasterVTable;
		}

		auto* player = RE::PlayerCharacter::GetSingleton();
		if (!player) {
			logger::warn("{}: PlayerCharacter not available, skip hook", logTag);
			return nullptr;
		}

		RE::ActorMagicCaster* sample = nullptr;
		for (auto slot : { RE::Actor::SlotTypes::kLeftHand, RE::Actor::SlotTypes::kRightHand }) {
			sample = player->magicCasters[slot];
			if (sample) {
				break;
			}
		}

		if (!sample) {
			logger::warn("{}: player has no ActorMagicCaster instances yet", logTag);
			return nullptr;
		}

		auto** vtblPtr = reinterpret_cast<std::uintptr_t**>(sample);
		if (!vtblPtr || !*vtblPtr) {
			logger::warn("{}: unable to resolve caster vtable", logTag);
			return nullptr;
		}

		g_playerCasterVTable = *vtblPtr;
		logger::info("{}: resolved ActorMagicCaster vtable {:p}", logTag, reinterpret_cast<const void*>(g_playerCasterVTable));
		return g_playerCasterVTable;
	}
}
