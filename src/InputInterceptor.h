#pragma once

namespace InputInterceptor
{
	void ConnectToConfig();
	void Install(const SKSE::LoadInterface* a_skse);
	void RefreshCastingState();
}
