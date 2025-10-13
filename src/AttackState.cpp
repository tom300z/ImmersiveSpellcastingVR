// AttackToggle.cpp (can be merged into your file if you like)
#include "RE/Skyrim.h"
#include "SKSE/SKSE.h"
#include "utils.h"

#include <atomic>
#include <chrono>
#include <thread>

namespace AttackState
{
	namespace
	{
		std::atomic_bool g_running{ false };
		std::thread g_thread;
		
		std::atomic_bool g_leftAttacking{ false };
		std::atomic_bool g_rightAttacking{ false };

		void SetDesiredAttackingState(bool left, bool attacking) {
			(left ? g_leftAttacking : g_rightAttacking).store(attacking, std::memory_order_relaxed);
		}

		// Run on UI thread
		void AddAttackButtonEvent(bool left, bool pressed)
		{
			auto* ue = RE::UserEvents::GetSingleton();
			auto* q = RE::BSInputEventQueue::GetSingleton();
			if (!ue || !q)
				return;

			const float value = pressed ? 1.0f : 0.0f;
			const float heldSec = pressed ? 0.0f : 0.1f;

			// Arg order per your ButtonEvent::Init signature: device, idCode, value, duration, userEvent
			q->AddButtonEvent(
				RE::INPUT_DEVICE::kGamepad,  // VR right-hand: kViveSecondary
				0,                           // idCode
				value,                       // 1.0=down, 0.0=up
				heldSec,                     // 0.0 new press; >0.0 signals release
				left? ue->leftAttack : ue->rightAttack              // BSFixedString user event
			);
		}

		// Ensures that the given hand is attacking or not, draws weapons if necessary
		void EnsureAttackingState(bool left, bool attacking)
		{
			auto player = RE::PlayerCharacter::GetSingleton();
			if (!player) {
				return;
			}

			// Draw weapons if necessary
			if (attacking && player->actorState2.weaponState != RE::WEAPON_STATE::kDrawn) {
				// Press trigger to draw weapons
				logger::debug("Drawing weapons...");
				SKSE::GetTaskInterface()->AddUITask([left]() { AddAttackButtonEvent(left, true); });

				// Wait for weapons
				while (player->actorState2.weaponState != RE::WEAPON_STATE::kDrawn) {
					std::this_thread::sleep_for(50ms);
				}
			}

			// Post to UI thread
			logger::debug("[AttackToggle] {} Attack {}", left ? "Left" : "Right", attacking ? "pressed" : "released");
			SKSE::GetTaskInterface()->AddUITask([left, attacking]() { AddAttackButtonEvent(left, attacking); });
		}

		void Loop()
		{
			using namespace std::chrono_literals;

			while (g_running.load(std::memory_order_relaxed)) {

				for (auto [isAttacking, isLeft] : {
						 std::pair{ g_leftAttacking.load(std::memory_order_relaxed), true },
						 std::pair{ g_rightAttacking.load(std::memory_order_relaxed), false } }) {
					if (utils::InGame()) {
						EnsureAttackingState(isLeft, isAttacking);
						std::this_thread::sleep_for(50ms);
					} /*else if (isAttacking) {
						// Stop attack button press
						EnsureAttackingState(isLeft, false);
					}*/
				}
			}
		}
	}  // namespace

	void Start()
	{
		if (g_running.exchange(true))
			return;
		g_thread = std::thread(Loop);
		spdlog::info("[AttackToggle] started");
	}

	void Stop()
	{
		if (!g_running.exchange(false))
			return;
		if (g_thread.joinable())
			g_thread.join();
		logger::info("[AttackToggle] stopped");
	}
}
