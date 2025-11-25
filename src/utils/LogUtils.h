#pragma once

#include <string_view>

namespace LogUtils
{
	void LogEquippedItems();
	void LogBytes(const void* ptr, int number = 5);
}
