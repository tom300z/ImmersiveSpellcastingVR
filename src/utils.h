#pragma once

#include "RE/Skyrim.h"
#include "SKSE/SKSE.h"

namespace utils
{
	bool InGame();
	void ExecuteConsoleCommand(std::string_view command);
}

namespace log_utils
{
	void LogEquippedItems();

	// --- Listener for equip events ---
	class EquipEventHandler : public RE::BSTEventSink<RE::TESEquipEvent>
	{
	public:
		static EquipEventHandler* GetSingleton()
		{
			static EquipEventHandler singleton;
			return std::addressof(singleton);
		}

		virtual RE::BSEventNotifyControl ProcessEvent(
			const RE::TESEquipEvent* a_event,
			RE::BSTEventSource<RE::TESEquipEvent>* a_eventSource) override
		{
			(void)a_eventSource; 
			if (a_event && a_event->actor.get() == RE::PlayerCharacter::GetSingleton()) {
				logger::info("EquipEvent fired → logging equipped items");
				LogEquippedItems();
			}
			return RE::BSEventNotifyControl::kContinue;
		}
	};
}

