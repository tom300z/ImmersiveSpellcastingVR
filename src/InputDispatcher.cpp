// AttackToggle.cpp (can be merged into your file if you like)
#include "RE/Skyrim.h"
#include "SKSE/SKSE.h"
#include "utils.h"
#include "InputDispatcher.h"

#include <atomic>
#include <chrono>
#include <thread>

namespace InputDispatcher
{
	std::atomic<bool> inputSuppressed{ false };

	// Run on UI thread
	void _AddAttackButtonEvent(bool left, bool pressed, float heldSecOverride = 0.0f)
	{
		if (inputSuppressed.load(std::memory_order_relaxed) && pressed) {
			return;
		}

		auto* ue = RE::UserEvents::GetSingleton();
		auto* q = RE::BSInputEventQueue::GetSingleton();
		if (!ue || !q)
			return;

		const float value = pressed ? 1.0f : 0.0f;
		const float heldSec = heldSecOverride ? heldSecOverride : pressed ? 0.0 : 0.1f;
		//logger::info("{} {}", left ? "Left" : "Right", pressed ? "press" : "unpress");

		// Arg order per your ButtonEvent::Init signature: device, idCode, value, duration, userEvent
		q->AddButtonEvent(
			RE::INPUT_DEVICE::kGamepad,              // VR right-hand: kViveSecondary
			0,                                       // idCode
			value,                                   // 1.0=down, 0.0=up
			heldSec,                                 // 0.0 new press; >0.0 signals release
			left ? ue->leftAttack : ue->rightAttack  // BSFixedString user event
		);
	}
	void AddAttackButtonEvent(bool left, bool pressed) {
		SKSE::GetTaskInterface()->AddUITask([left, pressed]() { _AddAttackButtonEvent(left, pressed); });

		if (pressed) {
			// Add another event after 50ms in case the player is dual casting (spells will o)
			std::thread([left] {
				std::this_thread::sleep_for(std::chrono::milliseconds(60));
				SKSE::GetTaskInterface()->AddUITask([left]() { _AddAttackButtonEvent(left, true, 0.06f); });
			}).detach();
		}
	}

	// Sometimes you have to simulate letting go of the input for a few ms to make sure the game starts casting a spell (For example shortly after casting fast healing)
	void RepressAttackButton(bool left)
	{
		std::thread([left] {
			AddAttackButtonEvent(left, false);
			std::this_thread::sleep_for(std::chrono::milliseconds(25));
			AddAttackButtonEvent(left, true);
		}).detach();
	}

	void SuppressInput(int ms) {
		std::thread([ms] {
			inputSuppressed = true;
			SKSE::GetTaskInterface()->AddUITask([]() { _AddAttackButtonEvent(true, false); });
			SKSE::GetTaskInterface()->AddUITask([ms]() { _AddAttackButtonEvent(false, false, ms); });
			std::this_thread::sleep_for(std::chrono::milliseconds(ms));
			inputSuppressed = false;
		}).detach();
	}
}
