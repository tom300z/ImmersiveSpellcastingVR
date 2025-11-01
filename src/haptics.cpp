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
		//logger::info("{} Hand Haptics: Initialized successfully.", handName);
	}

	void HandHaptics::ScheduleEvent(HapticEvent event) {
		{
			std::lock_guard lock(eventsMutex);
			if (event.interruptPulse || event.replaceScheduledEvents)
				events.clear();
			events.push_back(event);
		}
		if (event.interruptPulse) {
			// Immediately wake thread
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
			// Compute active event
			{
				std::lock_guard lock(eventsMutex);

				if (events.size() > 0) {
					const HapticEvent nextEvent = events.front();

					// Get next event on interrupt or if active event has no more pulses
					if (nextEvent.interruptPulse || activeEvent.pulses <= 0) {
						activeEvent = nextEvent;
						events.pop_front();
					}
				}
			}

			auto now = std::chrono::steady_clock::now();

			if (now >= nextPulse && (activeEvent.pulses > 0 || activeEvent.remainAfterCompletion )) {
				//auto time = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count();

				// Trigger haptic pulse
				if (activeEvent.pulseStrength > 0 && activeEvent.pulseStrength <= 1 && activeEvent.pulseInterval >= 5) {
					//logger::info("{} {} Pulse at {}ms", currentPulseStrength, handName, time);
					g_vrsystem->TriggerHapticPulse(
						!isLeftHand,
						activeEvent.pulseStrength
					);

				} else {
					//logger::info("IGNORED {} {} Pulse at {}ms", currentPulseStrength, handName, time);
				}

				if (activeEvent.pulses > 0) {
					activeEvent.pulses--;
				}

				// Schedule next pulse
				nextPulse = now + std::chrono::milliseconds(activeEvent.pulseInterval);
			}

			// Wait until next pulse or HapticState update or stop
			std::unique_lock lock(cv_mtx);
			if (activeEvent.pulseInterval <= 0 || (activeEvent.pulses <= 0 && !activeEvent.remainAfterCompletion)) {
				// If currentPulseInterval <= 0, wait indefinetly until notified
				cv.wait(lock);
			} else {
				cv.wait_until(lock, nextPulse);
			}

			// Wait while the worker is paused
			while (workerPaused.load(std::memory_order::relaxed)) {
				cv.wait(lock);
			}
		}
	};

	void HandHaptics::Pause(bool paused) {
		workerPaused.store(paused);

		// Wake up thread on unpause
		if (!paused) {
			cv.notify_one();
		}
	}

	static HandHaptics leftHH(true);
	static HandHaptics rightHH(false);

	HandHaptics* GetHandHaptics(bool getLeftHand)
	{
		return &(getLeftHand ? leftHH : rightHH);
	}

	void Pause(bool paused) {
		leftHH.Pause(paused);
		rightHH.Pause(paused);
	}
}
