#pragma once

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

	class HandHaptics
	{
	public:
		bool isLeftHand;
		std::string handName;

		HandHaptics(bool isLeftHand);

		void ScheduleEvent(HapticEvent event);

		void Pause(bool paused);

		~HandHaptics();

	private:
		RE::BSOpenVR* g_vrsystem;

		std::mutex eventsMutex;
		std::deque<HapticEvent> events;
		HapticEvent activeEvent = HapticEvent{};

		std::mutex cv_mtx;
		std::condition_variable cv;
		std::thread worker;
		std::atomic<bool> workerRunning = false;
		std::atomic<bool> workerPaused = false;

		void HapticThread();
	};

	extern HandHaptics leftHH;
	extern HandHaptics rightHH;

	HandHaptics* GetHandHaptics(bool getLeftHand);

	void Pause(bool paused);
}
