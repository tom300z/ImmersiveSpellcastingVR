#include "haptics.h"
#include <RE/Skyrim.h>
#include <chrono>
#include <thread>
#include <algorithm>
#include <cmath>
#include <atomic>
#include <sksevr_api.h>

using namespace std::chrono;

namespace Haptics
{
	HandHaptics::HandHaptics(bool isLeftHand) :
		isLeftHand(isLeftHand)
	{
		g_vrsystem = RE::BSOpenVR::GetSingleton();
		handName = isLeftHand ? "Left" : "Right";
		minInterval.store(std::chrono::milliseconds(0), std::memory_order::relaxed);
		Start();
		//logger::info("{} Hand Haptics: Initialized successfully.", handName);
	}

	void HandHaptics::ScheduleEvent(HapticEvent event)
	{
		{
			std::lock_guard lock(eventsMutex);
			if (event.interruptPulse || event.replaceScheduledEvents) {
				events.clear();
			}
			events.push_back(event);
		}

		const bool shouldNotify = event.interruptPulse || minInterval.load(std::memory_order::relaxed).count() <= 0;
		if (shouldNotify) {
			// Wake worker to process new input promptly
			Notify();
		}
	}

	void HandHaptics::Work()
	{
		{
			std::lock_guard lock(eventsMutex);

			if (!events.empty()) {
				const HapticEvent nextEvent = events.front();

				// Replace the active event if requested or once the current one is done
				if (nextEvent.interruptPulse || activeEvent.pulses <= 0) {
					activeEvent = nextEvent;
					events.pop_front();
				}
			}
		}

		const bool hasWork = (activeEvent.pulses > 0) || activeEvent.remainAfterCompletion;

		if (!hasWork) {
			minInterval.store(std::chrono::milliseconds(0), std::memory_order::relaxed);
			return;
		}

		if (activeEvent.pulseStrength > 0 && activeEvent.pulseStrength <= 1 && activeEvent.pulseInterval >= 5) {
			g_vrsystem->TriggerHapticPulse(!isLeftHand, activeEvent.pulseStrength);
		}

		if (activeEvent.pulses > 0) {
			activeEvent.pulses--;
		}

		if (activeEvent.pulseInterval > 0) {
			const auto interval = std::chrono::milliseconds(std::max(activeEvent.pulseInterval, 5));
			minInterval.store(interval, std::memory_order::relaxed);
		} else {
			minInterval.store(std::chrono::milliseconds(0), std::memory_order::relaxed);
		}
	}

	static HandHaptics leftHH(true);
	static HandHaptics rightHH(false);

	HandHaptics* GetHandHaptics(bool getLeftHand)
	{
		return &(getLeftHand ? leftHH : rightHH);
	}

	void Pause(bool paused)
	{
		if (paused) {
			leftHH.Stop();
			rightHH.Stop();
		} else {
			leftHH.Start();
			rightHH.Start();
		}
	}
}
