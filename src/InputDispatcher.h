#pragma once
#include <chrono>
#include <utils/TimedWorker.h>
#include <SpellChargeTracker.h>
#include "HandOrientation.h"

namespace InputDispatcher
{
	/// <summary>
	/// Class that automatically dispatches the input for the given hand based on declared state.
	/// </summary>
	class HandInputDispatcher : public Utils::TimedWorker
	{
	public:
		bool isLeftHand;
		std::string handName;

		HandInputDispatcher(bool isLeftHand);
		void OnCasterStateChanged();
		void RequestWork();

		void DeclareCasterState(bool casterActive);

		/// <summary>
		/// Suppresses all input until the caster has been declared inactive at least once. Can be used to prevent spells from firing until hand has been opened/closed once
		/// </summary>
		void SuppressUntilCasterInactive();
		std::atomic<bool> suppressUntilCasterInactive;

	private:
		void Work() override;
		void AddAttackButtonEvent(bool left, bool pressed, float heldSecOverride = 0.0f);

		// Current state of the respective MagicCaster
		std::atomic<SpellChargeTracker::ActualState>* currentCasterState;

		/// <summary>
		/// The caster being active means that the attack button is held to ensure the casters state is appropriate (anything other than kIdle kReleasing).
		/// This way inputs can be retriggered if the game ignored them for some reason until the caster has the desired state.
		/// </summary>
		std::atomic<bool> casterDeclaredActive;

		// Set to true whenever the caster state declaration changes, set to false after the change was processed.
		std::atomic<bool> casterDeclarationChanged;
		std::atomic<bool> workScheduled{ false };

		// Time point at which input changed last
		std::chrono::steady_clock::time_point currentInputStartTime = std::chrono::steady_clock::now();

		/// <summary>
		/// How long to wait for the caster state to change after sending an input
		/// </summary>
		constexpr static std::chrono::milliseconds kGracePeriod = std::chrono::milliseconds(200);

		/*
		// Duration to release trigger for during re-press
		constexpr static std::chrono::milliseconds kRePressReleaseDuration = std::chrono::milliseconds(50);

		// Set to true if the trigger is currently being re-pressed
		bool isRePressing = false;

		// Time at which the current re-press was started
		std::chrono::steady_clock::time_point currentRePressStartTime = std::chrono::steady_clock::now();
		*/
	};

	extern HandInputDispatcher leftDisp;
	extern HandInputDispatcher rightDisp;

	void Pause(bool paused);
}
