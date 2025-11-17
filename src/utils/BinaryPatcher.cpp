#include "utils/BinaryPatcher.h"

#include "REL/Relocation.h"
#include "REX/W32/KERNEL32.h"

#include <cstring>

namespace Utils
{
	BinaryPatcher::BinaryPatcher(
		std::string_view moduleName,
		std::string_view logPrefix,
		std::span<const PatchTarget> targets) :
		moduleName(moduleName),
		logPrefix(logPrefix)
	{
		this->targets.reserve(targets.size());
		for (const auto& target : targets) {
			this->targets.push_back(TargetState{ target, 0 });
		}
	}

	std::size_t BinaryPatcher::ResolveTargets()
	{
		if (moduleBase == 0) {
			const auto module = REX::W32::GetModuleHandleA(moduleName.c_str());
			if (!module) {
				logger::debug("{}: module {} not loaded; skipping patch", logPrefix, moduleName);
				return targets.size();
			}

			moduleBase = reinterpret_cast<std::uintptr_t>(module);
		}

		std::size_t mismatches = 0;
		for (auto& target : targets) {
			if (target.address != 0) {
				continue;
			}

			const auto address = moduleBase + target.data.offset;
			const auto* current_data = reinterpret_cast<const std::uint8_t*>(address);
			if (target.data.original.empty() ||
				std::memcmp(current_data, target.data.original.data(), target.data.original.size()) != 0) {
				logger::warn("{}: data mismatch for {}; skipping patch for that function", logPrefix, target.data.name);
				++mismatches;
				continue;
			}

			target.address = address;
		}

		return mismatches;
	}

	std::size_t BinaryPatcher::Apply(bool enabled)
	{
		const auto mismatches = ResolveTargets();

		for (auto& target : targets) {
			if (target.address == 0) {
				continue;
			}

			const auto& bytes = enabled ? target.data.patched : target.data.original;
			if (bytes.empty()) {
				continue;
			}

			REL::safe_write(
				target.address,
				bytes.data(),
				bytes.size());
		}

		return mismatches;
	}
}
