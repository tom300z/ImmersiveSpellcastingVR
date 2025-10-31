#pragma once

namespace InputInterceptor
{
	enum ButtonState
	{
		kUnknown,
		kUnpressed,
		kPressed
	};

	struct HandState
	{
		// The last known state of the casting button
		ButtonState lastCastingButtonState = ButtonState::kUnknown;

		// While enabled, the casting button input is hidden from the game
		bool hideCastingButtonFromGame = false;
	};

	void ConnectToConfig();
	void Install(const SKSE::LoadInterface* a_skse);
	void RefreshCastingState(bool forceRepressLeft = false, bool forceRepressRight = false);
}
