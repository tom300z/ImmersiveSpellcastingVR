#pragma once

#include "RE/Skyrim.h"
#include "SKSE/SKSE.h"

namespace Utils
{
	bool InGame();
	bool IsPlayerHoldingSpell(bool mainHand);
	bool InvertVRInputForSpell(RE::SpellItem* spell);
	void ExecuteConsoleCommand(std::string_view command);
	using MessageBoxResultCallbackFunc = std::function<void(unsigned int)>;
	void ShowMessageBox(const std::string& bodyText, const std::vector<std::string>& buttonTextValues = { "Ok" }, MessageBoxResultCallbackFunc callback = [](std::uint32_t) {});


	template <class T>
	class EventHandler : public RE::BSTEventSink<T>
	{
	public:
		using Callback = std::function<void(const T&)>;

		/// Construct a temporary event handler with a callback.
		explicit EventHandler(Callback a_callback) :
			callback(std::move(a_callback))
		{
			// Allocate a unique instance and store it in static storage
			instance = std::make_shared<EventHandler<T>>(*this);
		}

		/// Converts temporary object to a pointer usable by AddEventSink().
		operator RE::BSTEventSink<T>*()
		{
			return instance.get();
		}

		virtual RE::BSEventNotifyControl ProcessEvent(
			const T* a_event,
			RE::BSTEventSource<T>* a_eventSource) override
		{
			(void)a_eventSource;
			if (a_event && callback) {
				callback(*a_event);
			}
			return RE::BSEventNotifyControl::kContinue;
		}

	private:
		Callback callback;
		inline static std::shared_ptr<EventHandler<T>> instance{ nullptr };
	};

	namespace Animation
	{
		void DumpPlayerGraphVariables();
	}

	namespace Input
	{
		const char* GetOpenVRButtonName(std::uint32_t keyCode, vr::ETrackedControllerRole side = vr::ETrackedControllerRole::TrackedControllerRole_Invalid);
		RE::BSTArray<RE::ControlMap::UserEventMapping> GetActiveVRUserEventMapping(RE::UserEvents::INPUT_CONTEXT_ID context, bool leftHand);

		bool IsUsingIndexControllers();
	}

	namespace Setup
	{
		/// <summary>
		/// Performs the interactive setup & compatibility checks of the mod on game load. Can be called any number of times.
		/// </summary>
		void PerformInteractiveSetup();

		/// <summary>
		/// Sets up the mods logger. SHould be called once during SKSE query.
		/// </summary>
		void SetupLogger();
	}
}

namespace LogUtils
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

	void LogBytes(const void* ptr, int number = 5);
}
