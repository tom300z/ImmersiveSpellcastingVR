#pragma once

namespace Haptics
{
	class HandHaptics
	{
	public:
		bool isLeftHand;
		std::string handName;

		HandHaptics(bool isLeftHand);

		/* newPulseInterval: 5 - 200 ms, Interval between haptic pulses, 0 = off,
		newPulseStrength: 0.0f - 0.4f, 0.4f - 1.0f are supported but feel the same on index controllers.
		interruptCurrentPulse: if true, the current pulse will be interrupted and next pulse will happen immediately. Otherwise the next pulse will happen after the current one is finished
		*/
		void UpdateHapticState(int newPulseInterval = -1, float newPulseStrength = -1, bool interruptCurrentPulse = false);

		~HandHaptics();

	private:
		RE::BSOpenVR* g_vrsystem;

		std::atomic<int> pulseInterval{ 0 };   // 5 - 200 ms, Interval between haptic pulses, 0 = off,
		std::atomic<float> pulseStrength{ 0 };  // 0.0f - 0.4f, 0.4f - 1.0f are supported but feel the same on index controllers.

		std::mutex cv_mtx;
		std::condition_variable cv;
		std::thread worker;
		std::atomic<bool> workerRunning = false;

		void HapticThread();
	};

	extern HandHaptics leftHH;
	extern HandHaptics rightHH;

	HandHaptics* GetHandHaptics(bool getLeftHand);

	// Run this in a detached thread to play a simple haptic charge event for this hand (this isn't synced to the game obviously)
	void PlayHapticChargeEvent(HandHaptics* hand, float duration);
}
