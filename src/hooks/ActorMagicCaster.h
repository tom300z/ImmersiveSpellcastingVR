#pragma once

#include "RE/Skyrim.h"
#include "SKSE/SKSE.h"

#include <string_view>

namespace Hooks::ActorMagicCaster
{
	[[nodiscard]] std::uintptr_t* ResolvePlayerVTable(std::string_view logTag);

	template <class T>
	bool WriteHook(std::size_t slotIndex, std::string_view logTag, T*& original, T* replacement)
	{
		if (!replacement) {
			logger::warn("{}: replacement for slot {} is null", logTag, slotIndex);
			return false;
		}

		auto* vtbl = ResolvePlayerVTable(logTag);
		if (!vtbl) {
			return false;
		}

		original = reinterpret_cast<T*>(vtbl[slotIndex]);
		const auto hookAddr = reinterpret_cast<std::uintptr_t>(replacement);
		REL::safe_write(reinterpret_cast<std::uintptr_t>(&vtbl[slotIndex]), hookAddr);

		logger::info("{}: original slot[{}] {:p}", logTag, slotIndex, reinterpret_cast<const void*>(original));
		logger::info("{}: vtable slot[{}] patched -> {:p}", logTag, slotIndex, reinterpret_cast<const void*>(vtbl[slotIndex]));
		return true;
	}
}
