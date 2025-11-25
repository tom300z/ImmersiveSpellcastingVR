#include "SpellChargeTracker.h"

#include "CasterStateTracker.h"
#include "ConfigManager.h"
#include "Settings.h"
#include "compat/HapticSkyrimVR.h"
#include "haptics.h"
#include "utils.h"

#include <atomic>
#include <cmath>

namespace SpellChargeTracker
{
	namespace
	{
		using HapticEvent = Haptics::HapticEvent;

		std::atomic_bool g_hapticsEnabled{ true };
		std::uint64_t g_configListenerId{ 0 };
		std::uint64_t g_stateListenerId{ 0 };
		bool g_installed{ false };

		void StopAllHaptics()
		{
			if (auto* left = Haptics::GetHandHaptics(true)) {
				left->ScheduleEvent({});
			}
			if (auto* right = Haptics::GetHandHaptics(false)) {
				right->ScheduleEvent({});
			}
		}

		void ApplyHapticsEnabled(const Config::Value& value)
		{
			if (const auto* enabled = std::get_if<bool>(&value)) {
				const bool isEnabled = *enabled;
				g_hapticsEnabled.store(isEnabled, std::memory_order_relaxed);
				if (!isEnabled) {
					StopAllHaptics();
				}
				Compat::HapticSkyrimVR::DisableMagicHaptics(isEnabled);
			} else {
				logger::warn("SpellChargeTracker: Unsupported value type supplied for haptics enable configuration");
			}
		}

		void EnsureConfigListener()
		{
			if (g_configListenerId != 0) {
				return;
			}

			auto& config = Config::Manager::GetSingleton();
			ApplyHapticsEnabled(config.GetValue(Settings::kHapticsEnable));
			g_configListenerId = config.AddListener(
				[](std::string_view key, const Config::Value& value, [[maybe_unused]] Config::ChangeSource source) {
					if (key == Settings::kHapticsEnable) {
						ApplyHapticsEnabled(value);
					}
				});
		}

		void HandleStateChanged(const CasterStateTracker::StateChangedEvent& event)
		{
			if (!g_hapticsEnabled.load(std::memory_order_relaxed)) {
				return;
			}

			auto* caster = event.caster;
			auto* player = RE::PlayerCharacter::GetSingleton();
			if (!caster || !player || caster->actor != player) {
				return;
			}

			auto* handHaptics = Haptics::GetHandHaptics(event.orientation.isPhysicalLeft);
			if (!handHaptics) {
				return;
			}

			const auto newState = event.currentState;
			const auto previousState = event.previousState;

			if (newState == ActualState::kIdle) {
				handHaptics->ScheduleEvent({});
				return;
			}

			if (newState == ActualState::kStart || newState == ActualState::kCharging) {
				const float chargeProgress = (newState == ActualState::kCharging) ? (1.0f - (caster->castingTimer * 2.0f)) : 0.0f;
				handHaptics->ScheduleEvent({
					.pulseInterval = static_cast<int>(std::lerp(100.0f, 20.0f, chargeProgress)),
					.pulseStrength = static_cast<float>(std::lerp(0.0f, 1.0f, std::pow(chargeProgress, 6.0f))),
					.pulses = 0,
					.interruptPulse = (newState == ActualState::kStart)
				});
				return;
			}

			if (newState == ActualState::kHolding) {
				handHaptics->ScheduleEvent({ 50, 0.01f, 0, true });
				return;
			}

			if (newState == ActualState::kReleasing) {
				auto* spell = static_cast<RE::SpellItem*>(player->GetEquippedObject(!event.orientation.isMainHand));
				const bool interrupt = previousState != ActualState::kReleasing;
				if (Utils::InvertVRInputForSpell(spell)) {
					handHaptics->ScheduleEvent({ 30, 1, 0, interrupt });
				} else {
					handHaptics->ScheduleEvent({ 10, 1, 10, interrupt, false });
				}
				return;
			}
		}

		void EnsureStateListener()
		{
			if (g_stateListenerId != 0) {
				return;
			}

			g_stateListenerId = CasterStateTracker::AddListener(HandleStateChanged);
		}
	}  // namespace

	void Install()
	{
		if (g_installed) {
			return;
		}

		CasterStateTracker::Install();
		EnsureConfigListener();
		EnsureStateListener();

		g_installed = true;
	}
}
