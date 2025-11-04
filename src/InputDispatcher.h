#pragma once
#include <chrono>

namespace InputDispatcher
{
	void AddAttackButtonEvent(bool left, bool pressed);
	void RepressAttackButton(bool left);
	void SuppressInput(int ms); // Suppress input until
}
