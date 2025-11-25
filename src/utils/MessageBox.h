#pragma once

#include <functional>
#include <string>
#include <vector>

namespace Utils
{
	using MessageBoxResultCallbackFunc = std::function<void(unsigned int)>;

	void ShowMessageBox(
		const std::string& bodyText,
		const std::vector<std::string>& buttonTextValues = { "Ok" },
		MessageBoxResultCallbackFunc callback = [](std::uint32_t) {});
}
