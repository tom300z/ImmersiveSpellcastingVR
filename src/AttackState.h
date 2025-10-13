#pragma once

namespace AttackState
{
	void SetDesiredAttackingState(bool left, bool attacking);

	// Call this from main.cpp during plugin load
	void Start();
	void Stop();
}
