// AttackToggle.cpp (can be merged into your file if you like)
#include "RE/Skyrim.h"
#include "SKSE/SKSE.h"
#include "utils.h"
#include "InputDispatcher.h"
#include "CasterStateTracker.h"
#include "HandOrientation.h"

#include <atomic>
#include <chrono>
#include <thread>


namespace InputDispatcher
{
	std::atomic<bool> inputSuppressed{ false };

	namespace
	{
		std::uint64_t g_casterStateListenerId{ 0 };

		bool IsStateActive(SpellChargeTracker::ActualState state)
		{
			return !(state == SpellChargeTracker::ActualState::kIdle || state == SpellChargeTracker::ActualState::kReleasing);
		}

		void HandleCasterStateChanged(const CasterStateTracker::StateChangedEvent& event)
		{
			auto& dispatcher = event.orientation.isPhysicalLeft ? leftDisp : rightDisp;
			dispatcher.OnCasterStateChanged();
		}

		struct StateListenerRegistrar
		{
			StateListenerRegistrar()
			{
				g_casterStateListenerId = CasterStateTracker::AddListener(HandleCasterStateChanged);
			}

			~StateListenerRegistrar()
			{
				if (g_casterStateListenerId != 0) {
					CasterStateTracker::RemoveListener(g_casterStateListenerId);
				}
			}
		};
	}

	static StateListenerRegistrar g_stateListenerRegistrar;

	/*
	using StartCastFunc = bool (*)(RE::ActorMagicCaster*, RE::MagicItem*, void*, uint8_t);
	StartCastFunc StartCast = reinterpret_cast<StartCastFunc>(REL::Offset(0x550540).address());

	void SetCasting(RE::Actor* actor, bool mainHand, bool enabled) {
		//using func_t = bool (*)(RE::Actor*, RE::MagicSystem::CastingSource, bool);
		//REL::Relocation<func_t> func{ REL::ID(36925) };
		//return func(a_actor, a_source, a_enable);

		SKSE::GetTaskInterface()->AddUITask([actor, mainHand, enabled]() {
			if (Utils::InGame()) {
				const auto kSlot = mainHand ? RE::Actor::SlotTypes::kRightHand : RE::Actor::SlotTypes::kLeftHand;
				const auto kCastingSource = mainHand ? RE::MagicSystem::CastingSource::kRightHand : RE::MagicSystem::CastingSource::kLeftHand;

				if (enabled) {
					StartCast(
						actor->magicCasters[kSlot],
						actor->selectedSpells[kSlot],
						nullptr,
						0);
				} else {
					actor->GetMagicCaster(kCastingSource)->FinishCastImpl();
				}
			}
		});
	}
	*/


	// Run on UI thread
	void _AddAttackButtonEvent(bool left, bool pressed, float heldSecOverride = 0.0f)
	{
		if (inputSuppressed.load(std::memory_order_relaxed) && pressed) {
			return;
		}

		auto player = RE::PlayerCharacter::GetSingleton();
		const auto orientation = HandOrientation::FromPhysical(left);

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
			orientation.isMainHand ? ue->rightAttack : ue->leftAttack
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

	void HandInputDispatcher::RequestWork()
	{
		workScheduled.store(true, std::memory_order_relaxed);
		this->Notify();
	}

	void HandInputDispatcher::OnCasterStateChanged()
	{
		RequestWork();
	}

	void HandInputDispatcher::DeclareCasterState(bool casterActive) {
		// Store state if it changed
		const bool kOldCasterActive = casterDeclaredActive.exchange(casterActive, std::memory_order_relaxed);

		// If it changed, trigger a dispatch now and set the minInterval to 20ms to ensure it goes through.
		// Inputs typically take 10ms to result in a changed caster state so 20ms should be pretty efficient in case a repress is needed.
		if (kOldCasterActive != casterActive) {
			casterDeclarationChanged = true;
		}

		RequestWork();
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

	void HandInputDispatcher::Work()
	{
		const bool scheduledRun = workScheduled.exchange(false, std::memory_order_relaxed);
		const bool timedRun = minInterval.load(std::memory_order_relaxed).count() > 0;
		if (!scheduledRun && !timedRun) {
			return;
		}

		auto player = RE::PlayerCharacter::GetSingleton();
		const auto orientation = HandOrientation::FromPhysical(isLeftHand);

		if (!player) {
			minInterval.store(std::chrono::milliseconds(0), std::memory_order_relaxed);
			return;
		}

		// return if player is not holding spell or shouting
		if (!Utils::IsPlayerHoldingSpell(orientation.isMainHand) || player->GetMagicCaster(RE::MagicSystem::CastingSource::kOther)->state != RE::MagicCaster::State::kNone) {
			minInterval.store(std::chrono::milliseconds(0), std::memory_order_relaxed);
			return;
		}

		const bool kCasterDesiredActive = casterDeclaredActive.load(std::memory_order_relaxed);
		const SpellChargeTracker::ActualState kCasterState = currentCasterState->load(std::memory_order_relaxed);
		const bool kCasterActive = IsStateActive(kCasterState);

		if (suppressUntilCasterInactive.load(std::memory_order_relaxed)) {
			if (kCasterDesiredActive) {
				minInterval.store(std::chrono::milliseconds(0), std::memory_order_relaxed);
				return;
			}
			suppressUntilCasterInactive.store(false, std::memory_order_relaxed);
		}

		if (kCasterActive == kCasterDesiredActive && !casterDeclarationChanged.load(std::memory_order_relaxed)) {
			minInterval.store(std::chrono::milliseconds(0), std::memory_order_relaxed);
			return;
		}

		minInterval.store(std::chrono::milliseconds(20), std::memory_order_relaxed);
		const auto now = std::chrono::steady_clock::now();

		if (casterDeclarationChanged.exchange(false, std::memory_order_relaxed)) {
			currentInputStartTime = now;
			this->AddAttackButtonEvent(isLeftHand, kCasterDesiredActive);
			return;
		}

		const auto elapsed = now - currentInputStartTime;
		if (elapsed <= kGracePeriod) {
			this->AddAttackButtonEvent(isLeftHand, kCasterDesiredActive, std::chrono::duration<float>(elapsed).count());
			return;
		}

		this->AddAttackButtonEvent(isLeftHand, kCasterDesiredActive);
		currentInputStartTime = now;
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
