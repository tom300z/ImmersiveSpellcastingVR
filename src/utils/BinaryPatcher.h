#pragma once

#include <cstddef>
#include <cstdint>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace Utils
{
	class BinaryPatcher
	{
	public:
		struct PatchTarget
		{
			std::string name;
			std::uintptr_t offset;
			std::vector<std::uint8_t> original;
			std::vector<std::uint8_t> patched;
		};

		BinaryPatcher(
			std::string_view moduleName,
			std::string_view logPrefix,
			std::span<const PatchTarget> targets);

		std::size_t ResolveTargets();
		std::size_t Apply(bool enabled);

	private:
		struct TargetState
		{
			PatchTarget data;
			std::uintptr_t address{ 0 };
		};

		std::string moduleName;
		std::string logPrefix;
		std::vector<TargetState> targets;
		std::uintptr_t moduleBase{ 0 };
	};
}
