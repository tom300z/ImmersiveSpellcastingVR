#pragma once

#include "utils/TimedWorker.h"

#include <chrono>
#include <deque>
#include <mutex>
#include <string>

namespace Haptics
{
	struct HapticEvent
	{
		int pulseInterval = 0;
		float pulseStrength = 0;
		int pulses = 0;  // The (minimum) number of pulses to perform.

		bool interruptPulse = false;  // If true, all currently active/queued pulses will be skipped and this event will play immediately.
		bool remainAfterCompletion = true;  // If enabled the pulse parameters will remain active after the event is completed, until a new event is scheduled.
		bool replaceScheduledEvents = true;
	};

	class HandHaptics : public Utils::TimedWorker
	{
	public:
		bool isLeftHand;
		std::string handName;

		HandHaptics(bool isLeftHand);

		void ScheduleEvent(HapticEvent event);

	private:
		void Work() override;

		RE::BSOpenVR* g_vrsystem;

		std::mutex eventsMutex;
		std::deque<HapticEvent> events;
		HapticEvent activeEvent = HapticEvent{};
	};

	extern HandHaptics leftHH;
	extern HandHaptics rightHH;

	HandHaptics* GetHandHaptics(bool getLeftHand);

	void Pause(bool paused);
}
