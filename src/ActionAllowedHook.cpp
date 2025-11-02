#include "ActionAllowedHook.h"

#include "REL/Relocation.h"
#include "RE/B/ButtonEvent.h"
#include "RE/B/bhkCharacterController.h"
#include "RE/I/InputEvent.h"
#include "RE/J/JumpHandler.h"
#include "RE/P/PlayerCharacter.h"
#include "RE/P/PlayerControls.h"
#include "RE/P/PlayerControlsData.h"
#include "RE/U/UserEvents.h"

#include "SpellChargeTracker.h"
#include "InputInterceptor.h"
#include "utils.h"

namespace ActionAllowedHook
{
	namespace
	{
		using ActionAllowedFunc = bool (*)(RE::TESActionData*);
		using SpellState = SpellChargeTracker::ActualState;

		ActionAllowedFunc g_originalActionAllowed{ nullptr };
		bool g_installed{ false };

		bool ActionAllowedHook(RE::TESActionData* pActionData)
		{
			auto& actionID = pActionData->action->formEditorID;
			bool result = g_originalActionAllowed(pActionData);

			if (!result && actionID == "ActionJump") {
				auto* player = RE::PlayerCharacter::GetSingleton();
				if (player
					&& (((SpellState)player->magicCasters[RE::Actor::SlotTypes::kLeftHand]->state.get() != SpellState::kIdle) || ((SpellState)player->magicCasters[RE::Actor::SlotTypes::kRightHand]->state.get() != SpellState::kIdle))) {
					// If the player is trying to jump and one of the hands is currently charging/holding/casting a spell, allow the jump.
					// Hopefully this doesn't allow jumps in unwanted situations.
					result = true;
				}
			} else if (result && actionID == "ActionLand") {
				// Force the input interceptor to reread the controller state so that spells are charged if hand was closed during jump
				InputInterceptor::RefreshCastingState();
			} else if (result && (actionID == "ActionLeftInterrupt" || actionID == "ActionRightInterrupt")) {	
				// Force the input interceptor to reread the controller state so that spells are charged again after interrupting them
				auto* player = RE::PlayerCharacter::GetSingleton();
				if (player && player->isLeftHandMainHand) {
					// swap
					InputInterceptor::RefreshCastingState(actionID == "ActionRightInterrupt", actionID == "ActionLeftInterrupt");
				} else {
					InputInterceptor::RefreshCastingState(actionID == "ActionLeftInterrupt", actionID == "ActionRightInterrupt");
				}
			}

			//logger::info("{} -> {}", actionID.c_str(), result);
			return result;
		}
	}  // namespace

	void Install()
	{
		if (g_installed) {
			return;
		}


		constexpr REL::Offset kActionAllowedCallSiteOffset{ 0x72c5c1 };  // PlayerControls::subxxx call to TESActionData::ActionAllowed, Called when jumping/falling/landing
		constexpr REL::Offset kActionAllowedCallSiteOffset2{ 0x54902a };  // PlayerCharacter::DoAction_140544EA0 call to TESActionData::ActionAllowed, Called when attacking/ready/release
		REL::Relocation<std::uint8_t*> ActionAllowedCallSite{ kActionAllowedCallSiteOffset };

		auto& trampoline = SKSE::GetTrampoline();
		g_originalActionAllowed = (ActionAllowedFunc)trampoline.write_call<5>(kActionAllowedCallSiteOffset.address(), ActionAllowedHook);
		trampoline.write_call<5>(kActionAllowedCallSiteOffset2.address(), ActionAllowedHook);

		g_installed = true;
		logger::info("ActionAllowedHook: installed");
	}
}
