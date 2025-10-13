// AttackToggle.cpp (can be merged into your file if you like)
#include "RE/Skyrim.h"
#include "SKSE/SKSE.h"
#include "spdlog/spdlog.h"

#include <atomic>
#include <chrono>
#include <thread>

namespace AttackToggle
{
	namespace
	{
		std::atomic_bool g_running{ false };
		std::atomic_bool g_pressed{ false };
		std::thread g_thread;

		bool InGame()
		{
			auto* ui = RE::UI::GetSingleton();
			if (!ui || ui->GameIsPaused())
				return false;

			static constexpr std::string_view blockedMenus[] = {
				"Console", "InventoryMenu", "MagicMenu", "ContainerMenu", "BarterMenu",
				"Dialogue Menu", "MapMenu", "Journal Menu"
			};
			for (auto m : blockedMenus) {
				if (ui->IsMenuOpen(m))
					return false;
			}
			return true;
		}

		// Run on UI thread
		void SendRightAttackUI(bool press)
		{
			auto* ue = RE::UserEvents::GetSingleton();
			auto* q = RE::BSInputEventQueue::GetSingleton();
			if (!ue || !q)
				return;

			const float value = press ? 1.0f : 0.0f;
			const float heldSec = press ? 0.0f : 0.1f;

			// Arg order per your ButtonEvent::Init signature: device, idCode, value, duration, userEvent
			q->AddButtonEvent(
				RE::INPUT_DEVICE::kGamepad,  // VR right-hand: kViveSecondary
				0,                           // idCode
				value,                       // 1.0=down, 0.0=up
				heldSec,                     // 0.0 new press; >0.0 signals release
				ue->leftAttack              // BSFixedString user event
			);

			spdlog::info("[AttackToggle] RightAttack {}", press ? "pressed" : "released");
		}

		// Post to UI thread
		void PostSendRightAttack(bool press)
		{
			SKSE::GetTaskInterface()->AddUITask([press]() { SendRightAttackUI(press); });
		}

		void Loop()
		{
			using namespace std::chrono_literals;

			while (g_running.load(std::memory_order_relaxed)) {
				if (InGame()) {
					g_pressed = !g_pressed;
					PostSendRightAttack(g_pressed.load());
					std::this_thread::sleep_for(2s);
				} else {
					if (g_pressed.exchange(false)) {  // if it was pressed, release it
						PostSendRightAttack(false);
					}
					std::this_thread::sleep_for(200ms);
				}
			}

			// Cleanup: ensure released on exit
			if (g_pressed.exchange(false)) {
				PostSendRightAttack(false);
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
		spdlog::info("[AttackToggle] stopped");
	}
}
