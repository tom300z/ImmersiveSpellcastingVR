#pragma once

#include <string_view>

namespace Settings
{
	using namespace std::string_view_literals;

	inline constexpr auto kCastingInputMethod = "CastingInputMethod"sv;
	inline constexpr auto kShowBindingWarning = "ShowBindingWarning"sv;
}

