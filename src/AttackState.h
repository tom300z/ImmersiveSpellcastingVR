#pragma once
#include <chrono>

namespace AttackState
{
	void AddAttackButtonEvent(bool left, bool pressed);
	void RepressAttackButton(bool left);
	void SuppressInput(int ms); // Suppress input until
}
