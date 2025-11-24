#include "InputInterceptor.h"

#include <array>
#include <atomic>
#include <cctype>
#include <cstdint>
#include <limits>
#include <string>
#include <string_view>
#include <vector>

#include "InputDispatcher.h"
#include "ConfigManager.h"
#include "Settings.h"
#include "sksevr_api.h"
#include "utils.h"
#include "haptics.h"
#include "math.h"
#include "InputInterceptor.h"

namespace InputInterceptor
{
	vr::EVRButtonId g_castingButtonId = vr::EVRButtonId::k_EButton_Grip;
	bool g_castingButtonTouch = false;
	std::atomic_bool g_refreshLeft{ false };
	std::atomic_bool g_refreshRight{ false };

	HandState g_leftHandState = HandState();
	HandState g_rightHandState = HandState();

	namespace
	{
		std::atomic_bool g_installed{ false };
		std::atomic_bool g_inputEnabled{ true };
		std::uint64_t g_configListenerId{ 0 };

		struct ButtonMapping
		{
			std::string_view name;
			vr::EVRButtonId id;
		};

		void ApplyCastingInputMethod(const Config::Value& value)
		{
			std::string congfiguredInputMethod = *std::get_if<std::string>(&value);
			vr::EVRButtonId newButton = vr::EVRButtonId::k_EButton_Grip;
			bool newButtonTouch = false;
			if (congfiguredInputMethod == "grip_press") {
				newButton = vr::EVRButtonId::k_EButton_Grip;
				newButtonTouch = false;
			} else if (congfiguredInputMethod == "grip_touch") {
				newButton = vr::EVRButtonId::k_EButton_Grip;
				newButtonTouch = true;
			} else {
				logger::warn("Unsupported value type supplied for casting button configuration");
			}

			if (newButton != g_castingButtonId || newButtonTouch != g_castingButtonTouch) {

				g_castingButtonId = newButton;
				g_castingButtonTouch = newButtonTouch;

				g_leftHandState.lastCastingButtonState = ButtonState::kUnknown;
				g_rightHandState.lastCastingButtonState = ButtonState::kUnknown;

				const auto name = Utils::Input::GetOpenVRButtonName(static_cast<std::uint32_t>(newButton));
				logger::info("Casting input method updated to {} {}", name ? name : "unknown", newButtonTouch ? "Touch" : "Grip");
			}
		}

		void ApplyInputEnabled(const Config::Value& value)
		{
			if (const auto* enabled = std::get_if<bool>(&value)) {
				const bool previous = g_inputEnabled.exchange(*enabled);
				if (!*enabled) {
					g_leftHandState.lastCastingButtonState = ButtonState::kUnknown;
					g_leftHandState.hideCastingButtonFromGame = false;
					g_rightHandState.lastCastingButtonState = ButtonState::kUnknown;
					g_rightHandState.hideCastingButtonFromGame = false;
				} else if (!previous && *enabled) {
					RefreshCastingState();
				}
			} else {
				logger::warn("Unsupported value type supplied for input enable configuration");
			}
		}

		void ProcessCastingButtonState(bool isLeftHand, bool castingButtonActivated, bool forceDispatch = false)
		{
			if (!g_inputEnabled.load(std::memory_order::relaxed)) {
				return;
			}

			const ButtonState newState = castingButtonActivated ? ButtonState::kPressed : ButtonState::kUnpressed;
			HandState* handState = isLeftHand ? &g_leftHandState : &g_rightHandState;

			if (!forceDispatch) {
				if (
					(handState->lastCastingButtonState != ButtonState::kUnknown)
					&& (handState->lastCastingButtonState == newState)
				) {
					return;
				}
			}


			auto player = RE::PlayerCharacter::GetSingleton();
			const bool isMainHand = RE::BSOpenVRControllerDevice::IsLeftHandedMode() ? isLeftHand : !isLeftHand;  // in-game notion of “main hand”
			InputDispatcher::HandInputDispatcher& kDispatcher = (isLeftHand ? InputDispatcher::leftDisp : InputDispatcher::rightDisp);

			if ((
				player                                                                          // Player exists
				&& Utils::InGame()                                                              // player is in Game
				&& Utils::IsPlayerHoldingSpell(isMainHand)                                      // is holding spell
			)) {
				auto spell = static_cast<RE::SpellItem*>(player->GetEquippedObject(!isMainHand));
				bool invertInput = Utils::InvertVRInputForSpell(spell);
				const bool desiredAttackPressed = invertInput ? !castingButtonActivated : castingButtonActivated;

				// Declare desired caster state
				kDispatcher.DeclareCasterState(desiredAttackPressed);


				// Hide original input from game, if it is currently pressed
				if (newState == ButtonState::kPressed) {
					handState->hideCastingButtonFromGame = true;
				}

				(isLeftHand? g_refreshLeft : g_refreshRight).store(false, std::memory_order_relaxed);
			} else {
				// Unhide original input from game if the casting button is not pressed (to avoid unwanted inputs when opening menus)
				if (newState == ButtonState::kUnpressed || handState->lastCastingButtonState == ButtonState::kUnpressed) {
					handState->hideCastingButtonFromGame = false;
				}
			}

			// Update last casting button state
			handState->lastCastingButtonState = newState;
		}
	}

	void ControllerCallback(uint32_t unControllerDeviceIndex, vr::VRControllerState001_t* pControllerState, [[maybe_unused]] uint32_t unControllerStateSize, [[maybe_unused]] bool& state)
	{
		if (!g_inputEnabled.load(std::memory_order::relaxed)) {
			return;
		}

		// Skip if this is not for a hand
		auto role = vr::VRSystem()->GetControllerRoleForTrackedDeviceIndex(unControllerDeviceIndex);
		if (!(role == vr::TrackedControllerRole_LeftHand || role == vr::TrackedControllerRole_RightHand)) {
			return;
		}
		const bool isLeftHand = role == vr::TrackedControllerRole_LeftHand;

		// Get the right mask for press or touch
		uint64_t* buttonActivationMask = g_castingButtonTouch ? &pControllerState->ulButtonTouched : &pControllerState->ulButtonPressed;

		// Check if the casting button is pressed/touched
		const bool castingButtonActivated = (*buttonActivationMask & vr::ButtonMaskFromId(g_castingButtonId));

		// Process button state, force dispatch if refresh was scheduled, otherwise only process on changed state
		ProcessCastingButtonState(isLeftHand, castingButtonActivated, (isLeftHand ? g_refreshLeft : g_refreshRight).load(std::memory_order_relaxed));

		// Hide the casting button press from the game if it is supposed to be hidden
		HandState* handState = isLeftHand ? &g_leftHandState : &g_rightHandState;
		if (handState->hideCastingButtonFromGame) {
			*buttonActivationMask &= ~vr::ButtonMaskFromId(g_castingButtonId);
		}
	}

	// This needs to be called manually after the config was loaded
	void ConnectToConfig() {
		ApplyCastingInputMethod(Config::Manager::GetSingleton().GetValue(Settings::kInputMethod));
		ApplyInputEnabled(Config::Manager::GetSingleton().GetValue(Settings::kInputEnable));
		if (g_configListenerId == 0) {
			g_configListenerId = Config::Manager::GetSingleton().AddListener(
				[](std::string_view key, const Config::Value& value, [[maybe_unused]] Config::ChangeSource source) {
					if (key == Settings::kInputMethod) {
						ApplyCastingInputMethod(value);
					} else if (key == Settings::kInputEnable) {
						ApplyInputEnabled(value);
					}
				});
		}
	}

	void RefreshCastingState()
	{
		g_refreshLeft.store(true);
		g_refreshRight.store(true);
	}

	void Install(const SKSE::LoadInterface* a_skse)
	{
		// Skip if already installed
		if (g_installed.exchange(true)) {
			return;
		}

		auto g_vrinterface = (SKSEVRInterface*)a_skse->QueryInterface(kInterface_VR);
		g_vrinterface->RegisterForControllerState(a_skse->GetPluginHandle(), std::numeric_limits<std::int32_t>::max(), ControllerCallback);
	}
}
