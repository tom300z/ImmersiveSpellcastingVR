// AttackToggle.cpp (can be merged into your file if you like)
#include "RE/Skyrim.h"
#include "SKSE/SKSE.h"
#include "utils.h"
#include "InputDispatcher.h"

#include <atomic>
#include <chrono>
#include <thread>

namespace InputDispatcher
{
	std::atomic<bool> inputSuppressed{ false };

	// Run on UI thread
	void _AddAttackButtonEvent(bool left, bool pressed, float heldSecOverride = 0.0f)
	{
		if (inputSuppressed.load(std::memory_order_relaxed) && pressed) {
			return;
		}

		auto player = RE::PlayerCharacter::GetSingleton();
		const bool isMainHand = RE::BSOpenVRControllerDevice::IsLeftHandedMode() ? left : !left;              // in-game notion of “main hand”

		auto* ue = RE::UserEvents::GetSingleton();
		auto* q = RE::BSInputEventQueue::GetSingleton();
		if (!ue || !q)
			return;

		const float value = pressed ? 1.0f : 0.0f;
		const float heldSec = heldSecOverride ? heldSecOverride : pressed ? 0.0 : 0.1f;

		// Arg order per your ButtonEvent::Init signature: device, idCode, value, duration, userEvent
		q->AddButtonEvent(
			RE::INPUT_DEVICE::kGamepad,               // VR right-hand: kViveSecondary
			0,                                        // idCode
			value,                                    // 1.0=down, 0.0=up
			heldSec,                                  // 0.0 new press; >0.0 signals release
			isMainHand ? ue->rightAttack : ue->leftAttack  //ue->leftAttack : ue->rightAttack,  // BSFixedString user event
		);
	}

	HandInputDispatcher::HandInputDispatcher(bool isLeftHand) :
		isLeftHand(isLeftHand)
	{
		handName = isLeftHand ? "Left" : "Right";
		currentCasterState = isLeftHand ? &SpellChargeTracker::lastLeftHandState : &SpellChargeTracker::lastRightHandState;

		minInterval.store(std::chrono::milliseconds(0), std::memory_order::relaxed);
		Start();
	}

	void HandInputDispatcher::DeclareCasterState(bool casterActive) {
		// Store state if it changed
		const bool kOldCasterActive = casterDeclaredActive.exchange(casterActive, std::memory_order_relaxed);

		// If it changed, trigger a dispatch now and set the minInterval to 20ms to ensure it goes through.
		// Inputs typically take 10ms to result in a changed caster state so 20ms should be pretty efficient in case a repress is needed.
		if (kOldCasterActive != casterActive) {
			auto time = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count();
			//logger::trace("{}: {} caster declared {}", time, isLeftHand ? "Left" : "Right", casterActive ? "active" : "inactive");
			casterDeclarationChanged = true;
			this->Notify();
		}

	}

	void HandInputDispatcher::SuppressUntilCasterInactive()
	{
		suppressUntilCasterInactive.store(true);
	}

	void HandInputDispatcher::AddAttackButtonEvent(bool left, bool pressed, float heldSecOverride)
	{
		//auto time = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count();
		//logger::trace("{}: {} {}", time, left ? "Left" : "Right", pressed ? "press" : "unpress");
		SKSE::GetTaskInterface()->AddUITask([left, pressed, heldSecOverride]() { _AddAttackButtonEvent(left, pressed, heldSecOverride); });
	}

	void HandInputDispatcher::Work() {
		auto player = RE::PlayerCharacter::GetSingleton();
		const bool isMainHand = RE::BSOpenVRControllerDevice::IsLeftHandedMode() ? isLeftHand : !isLeftHand;  // in-game notion of “main hand”

		// release & return if player is not holding spell
		if (!(player
			&& player->actorState2.weaponState == RE::WEAPON_STATE::kDrawn  // has the weapons drawn
			&& player->GetEquippedObject(!isMainHand)                   // Hand has object equipped
			&& player->GetEquippedObject(!isMainHand)->GetFormType() == RE::FormType::Spell
			&& player->GetMagicCaster(RE::MagicSystem::CastingSource::kOther)
			&& player->GetMagicCaster(RE::MagicSystem::CastingSource::kOther)->state == RE::MagicCaster::State::kNone)) {
			return;
		}

		// Determine desired and currend caster state
		const bool kCasterDesiredActive = casterDeclaredActive.load(std::memory_order_relaxed);
		const SpellChargeTracker::ActualState kCasterState = currentCasterState->load(std::memory_order_relaxed);
		const bool kCasterActive = !(kCasterState == SpellChargeTracker::ActualState::kIdle || kCasterState == SpellChargeTracker::ActualState::kReleasing);

		// Return if suppressUntilCasterInactive is true but caster is desired active
		if (suppressUntilCasterInactive.load(std::memory_order_relaxed)) {
			if (kCasterDesiredActive) {
				return;
			} else {
				suppressUntilCasterInactive.store(false, std::memory_order_relaxed);
			}
		}


		// Return if caster already has desired state and the declaration was not just updated. Also slow down the worker until a new state is declared
		if (kCasterActive == kCasterDesiredActive && !casterDeclarationChanged.load(std::memory_order_relaxed))
		{
			// The worker still runs every 100ms in case the caster state drifts (for example after a fast healing spell kCasterActive lingers true for a short period after releasing.)
			minInterval.store(std::chrono::milliseconds(100), std::memory_order::relaxed);
			return;
		}

		// Dispatch input to reconciliate caster state. Work every 20ms during reconciliation
		minInterval.store(std::chrono::milliseconds(20), std::memory_order::relaxed);

		// Compute the appropriate input
		const auto now = std::chrono::steady_clock::now();

		if (casterDeclarationChanged.exchange(false, std::memory_order_relaxed)) {
			// Start new input if the caster state declaration changed
			currentInputStartTime = now;
		}
		std::chrono::duration<float> timeSinceCurrentInput = now - currentInputStartTime;

		if (timeSinceCurrentInput <= kGracePeriod) {
			// If in grace period, just send the attack button event with the real time held
			this->AddAttackButtonEvent(isLeftHand, kCasterDesiredActive, timeSinceCurrentInput.count());
		} else {
			// If caster still hasn't adjusted after the grace period, we need to repress (or release)
			if (kCasterDesiredActive) {
				// If trigger should be pressed,  before repressing it for another gracePeriod. Repeat this until the caster reaches it's desired state which causes this code to no longer be reached.
				this->AddAttackButtonEvent(isLeftHand, true);

				/* TODO: Readd more complex repress logic if it is really necessary
				if (!isRePressing) {
					// Release trigger, store the repress & repress start time in state, wait kRePressReleaseDuration
					isRePressing = true;
					currentRePressStartTime = now + kRePressReleaseDuration;
					minInterval = kRePressReleaseDuration;
					logger::trace("{}: {} Re-press: released trigger...", std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count(), handName);
				} else {
					// Re-press trigger for grace period, then, stop the re-press if it was unsuccessful. (Leading to another release & re-press)
					std::chrono::duration<float> timeSinceRePressStart = now - currentRePressStartTime; // TODO Must be 0 on first hit
					if (timeSinceRePressStart <= kGracePeriod) {
						this->AddAttackButtonEvent(isLeftHand, true, timeSinceRePressStart.count());
						logger::trace("{}: {} Re-press: pressed trigger for {}s...", std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count(), handName, timeSinceRePressStart.count());
					} else {
						isRePressing = false;
					}
				}*/
			}
		}
	}

	HandInputDispatcher leftDisp(true);
	HandInputDispatcher rightDisp(false);

	void Pause(bool paused)
	{
		if (paused) {
			leftDisp.Stop();
			rightDisp.Stop();
		} else {
			leftDisp.Start();
			rightDisp.Start();
		}
	}
}
