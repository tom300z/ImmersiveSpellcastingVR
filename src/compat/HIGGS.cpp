#include "HIGGS.h"

#include "REL/Relocation.h"
#include "REX/W32/KERNEL32.h"

#include <array>
#include <cstring>
#include <span>
#include <ConfigManager.h>
#include <Settings.h>

namespace Compat::HIGGSUseTouchForGrip {
	// Patcher for HIGGS 1.10.6
	const char* kModuleName = "higgs_vr.dll";
	std::uint64_t g_configListenerId{ 0 };
	bool g_installed = false;

	struct PatchTarget
	{
		const char* name;
		std::uintptr_t offset;
		std::vector<std::uint8_t> original;
		std::vector<std::uint8_t> patched;

		std::uintptr_t address{ 0 };
	};

	namespace
	{
		std::array<PatchTarget, 4> kTargetsTemplate{
			PatchTarget{
				.name = "gripDownPatch",
				.offset = 0x278cc,
				.original{ 0xf6, 0x47, 0x08, 0x04 },
				.patched{ 0xf6, 0x47, 0x10, 0x04 } },
			PatchTarget{
				.name = "leewaySuppressGrip",
				.offset = 0x27bb5,
				.original{ 0x48, 0x83, 0x67, 0x08, 0xfb },
				.patched{ 0x48, 0x83, 0x67, 0x10, 0xfb } },
			PatchTarget{
				.name = "blockSuppressGrip",
				.offset = 0x27c38,
				.original{ 0x48, 0x83, 0x67, 0x08, 0xfb },
				.patched{ 0x48, 0x83, 0x67, 0x10, 0xfb } },
			PatchTarget{
				.name = "forceSuppressGrip",
				.offset = 0x27c71,
				.original{ 0x48, 0x83, 0x4f, 0x08, 0x04 },
				.patched{ 0x48, 0x83, 0x4f, 0x10, 0x04 } }
		};

		std::array<PatchTarget, kTargetsTemplate.size()> g_targets{ kTargetsTemplate };
		std::uintptr_t g_moduleBase{ 0 };

		short resolve_targets()
		{
			if (g_moduleBase == 0) {
				const auto module = REX::W32::GetModuleHandleA(kModuleName);
				if (!module) {
					logger::debug("Compat::HIGGSUseTouchForGrip: module {} not loaded; skipping patch", kModuleName);
					return g_targets.size();
				}

				g_moduleBase = reinterpret_cast<std::uintptr_t>(module);
			}

			std::size_t mismatches = 0;
			for (auto& target : g_targets) {
				if (target.address) {
					continue;
				}

				const auto address = g_moduleBase + target.offset;
				const auto* current_data = reinterpret_cast<const std::uint8_t*>(address);

				if (std::memcmp(current_data, target.original.data(), target.original.size()) != 0) {
					logger::warn("Compat::HIGGSUseTouchForGrip: data mismatch for {}; skipping patch for that function. you probably dont have HIGGS 1.10.6 installed.", target.name);
					++mismatches;
					continue;
				}

				target.address = address;
			}

			return mismatches;
		}
	}

	/// <summary>
	/// (Un)Applies the patches and returns the number of mismatches. ret 0 == Success.
	/// </summary>
	/// <param name="enabled"> Whether to apply or unapply the patch.</param>
	/// <returns></returns>
	void Apply(bool enabled)
	{
		// only for HIGGS 1.10.6

		const auto mismatches = resolve_targets();

		for (auto& target : g_targets) {
			if (target.address == 0) {
				// Skip targets that were not resolved
				continue;
			}

			if (enabled) {
				// Apply patch
				REL::safe_write(
					target.address,
					target.patched.data(),
					target.patched.size());
			} else {
				// Unapply patch
				REL::safe_write(
					target.address,
					target.original.data(),
					target.original.size());
			}
		}
	}

	
	void Install()
	{
		if (resolve_targets()) {
			// Could not be installed due to missing higgs or incompatibility. 
			return;
		}
		if (g_configListenerId != 0) {
			return;
		}

		auto& config = Config::Manager::GetSingleton();
		Apply(std::get<bool>(config.GetValue(Settings::kInputHackHiggsTouchInput)));
		g_configListenerId = config.AddListener(
			[](std::string_view key, const Config::Value& value, [[maybe_unused]] Config::ChangeSource source) {
				if (key == Settings::kInputHackHiggsTouchInput) {
					Apply(std::get<bool>(value));
				}
			});

		g_installed = true;
	}
}
