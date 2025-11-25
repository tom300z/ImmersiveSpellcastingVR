#pragma once

#include <string_view>
#include <span>

#include "ConfigManager.h"

namespace Settings
{
	using namespace std::string_view_literals;

	inline constexpr auto kInputMethod = "CastingInputMethod"sv;
	inline constexpr auto kInputShowBindingWarning = "ShowBindingWarning"sv;
	inline constexpr auto kInputEnable = "InputEnable"sv;
	inline constexpr auto kInputCastAfterMenuExit = "InputCastAfterMenuExit"sv;
	inline constexpr auto kInputHackHiggsTouchInput = "InputHackHiggsTouchInput"sv;

	inline constexpr auto kHapticsEnable = "HapticsEnable"sv;

	std::span<const Config::SettingDefinition> GetSettingDefinitions();
}
