// AttackToggle.cpp (can be merged into your file if you like)
#include "RE/Skyrim.h"
#include "SKSE/SKSE.h"
#include "utils.h"

#include <atomic>
#include <chrono>
#include <thread>

namespace AttackState
{
	/*
	std::atomic_bool g_running{ false };
	std::thread g_thread;

	std::atomic_bool g_leftAttacking{ false };
	std::atomic_bool g_rightAttacking{ false };
	*/

	// Run on UI thread
	void _AddAttackButtonEvent(bool left, bool pressed)
	{
		auto* ue = RE::UserEvents::GetSingleton();
		auto* q = RE::BSInputEventQueue::GetSingleton();
		if (!ue || !q)
			return;

		const float value = pressed ? 1.0f : 0.0f;
		const float heldSec = pressed ? 0.0f : 0.1f;
		//logger::info("{} {}", left ? "Left" : "Right", pressed ? "press" : "unpress");

		// Arg order per your ButtonEvent::Init signature: device, idCode, value, duration, userEvent
		q->AddButtonEvent(
			RE::INPUT_DEVICE::kGamepad,  // VR right-hand: kViveSecondary
			0,                           // idCode
			value,                       // 1.0=down, 0.0=up
			heldSec,                     // 0.0 new press; >0.0 signals release
			left? ue->leftAttack : ue->rightAttack              // BSFixedString user event
		);
	}
	void AddAttackButtonEvent(bool left, bool pressed) {
		SKSE::GetTaskInterface()->AddUITask([left, pressed]() { _AddAttackButtonEvent(left, pressed); });
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
}
