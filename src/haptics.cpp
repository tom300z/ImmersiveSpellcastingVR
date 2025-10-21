#include "haptics.h"
#include <RE/Skyrim.h>
#include <chrono>
#include <thread>
#include <algorithm>
#include <cmath>
#include <sksevr_api.h>

using namespace std::chrono;

namespace Haptics
{
	HandHaptics::HandHaptics(bool isLeftHand) :
		isLeftHand(isLeftHand) {

		g_vrsystem = RE::BSOpenVR::GetSingleton();
		handName = isLeftHand ? "Left" : "Right";
		worker = std::thread(&HandHaptics::HapticThread, this);
		logger::info("{} Hand Haptics: Initialized successfully.", handName);
	}

	void HandHaptics::UpdateHapticState(int newPulseInterval, float newPulseStrength, bool interruptCurrentPulse)
	{
		/* newPulseInterval: 5 - 200 ms, Interval between haptic pulses, 0 = off,
		newPulseStrength: 0.0f - 0.4f, 0.4f - 1.0f are supported but feel the same on index controllers.
		interruptCurrentPulse: if true, the current pulse will be interrupted and next pulse will happen immediately. Otherwise the next pulse will happen after the current one is finished
		*/
		if (newPulseInterval >= 0) {
			pulseInterval.store(newPulseInterval, std::memory_order_relaxed);
		}
		if (newPulseStrength >= 0 && newPulseStrength <= 1) {
			pulseStrength.store(newPulseStrength, std::memory_order_relaxed);
		}
		if (interruptCurrentPulse) {
			logger::info("{} Hand Haptic Pulse interrupted.", handName);
			cv.notify_one();
		}
	}

	HandHaptics::~HandHaptics()
	{
		workerRunning = false;
		cv.notify_one();
		worker.join();
	}

	void HandHaptics::HapticThread()
	{
		workerRunning = true;
		auto nextPulse = std::chrono::steady_clock::now();

		while (workerRunning.load(std::memory_order::relaxed)) {
			auto now = std::chrono::steady_clock::now();
			auto currentPulseInterval = pulseInterval.load(std::memory_order::relaxed);
			auto currentPulseStrength = pulseStrength.load(std::memory_order::relaxed);

			if (now >= nextPulse) {
				auto time = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count();

				// Trigger haptic pulse
				if (currentPulseStrength > 0 && currentPulseStrength <= 1) {
					logger::info("{} {} Pulse at {}ms", currentPulseStrength, handName, time);
					g_vrsystem->TriggerHapticPulse(
						!isLeftHand,
						currentPulseStrength
					);
				} else {
					logger::info("IGNORED {} {} Pulse at {}ms", currentPulseStrength, handName, time);
				}

				// Schedule next pulse
				nextPulse = now + std::chrono::milliseconds(currentPulseInterval);
			}

			// Wait until next pulse or HapticState update or stop
			std::unique_lock lock(cv_mtx);
			if (currentPulseInterval <= 0) {
				// If currentPulseInterval <= 0, wait indefinetly until notified
				cv.wait(lock);
			} else {
				cv.wait_until(lock, nextPulse);
			}
		}
	};



	// Run this in a detached thread to play a simple haptic charge event for this hand (this isn't synced to the game obviously)
	void PlayHapticChargeEvent(HandHaptics* hand, float duration)
	{
		auto charge_start = std::chrono::steady_clock::now();
		auto charge_end = charge_start + std::chrono::duration<float>(duration);

		float new_charge = 0;
		int updateNumber = 0;

		while (new_charge < 1) {
			new_charge = std::clamp(
				std::chrono::duration<float>(std::chrono::steady_clock::now() - charge_start).count() /
					std::chrono::duration<float>(charge_end - charge_start).count(),
				0.0f, 1.0f);

			hand->UpdateHapticState(
				std::lerp(100, 20, new_charge),                   // Fixed to 20ms for now
				std::pow(new_charge, 6),  // Using pow makes the interpolation exponential instead of linear
				(updateNumber == 0)                              // Interrupt current pulse on first update
			);
			logger::info("New Charge: {}, Duration: {}", std::pow(new_charge, 6), std::lerp(0.0f, 0.5f, std::pow(new_charge, 6)));
			updateNumber++;
			std::this_thread::sleep_for(std::chrono::milliseconds(5));
		}
		hand->UpdateHapticState(20, 0.005, true); // Stop haptics

	}
	static HandHaptics leftHH(true);
	static HandHaptics rightHH(false);

	HandHaptics* GetHandHaptics(bool getLeftHand)
	{
		return &(getLeftHand ? leftHH : rightHH);
	}
}
