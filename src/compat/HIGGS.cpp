#include "HIGGS.h"

#include "utils/BinaryPatcher.h"

#include <array>
#include <span>
#include <ConfigManager.h>
#include <Settings.h>

namespace Compat::HIGGSUseTouchForGrip {
	// Patcher for HIGGS 1.10.6
	const char* kModuleName = "higgs_vr.dll";
	std::uint64_t g_configListenerId{ 0 };
	bool g_installed = false;

	namespace
	{
		std::array<Utils::BinaryPatcher::PatchTarget, 4> kTargets{
			Utils::BinaryPatcher::PatchTarget{
				.name = "gripDownPatch",
				.offset = 0x278cc,
				.original{ 0xf6, 0x47, 0x08, 0x04 },
				.patched{ 0xf6, 0x47, 0x10, 0x04 } },
			Utils::BinaryPatcher::PatchTarget{
				.name = "leewaySuppressGrip",
				.offset = 0x27bb5,
				.original{ 0x48, 0x83, 0x67, 0x08, 0xfb },
				.patched{ 0x48, 0x83, 0x67, 0x10, 0xfb } },
			Utils::BinaryPatcher::PatchTarget{
				.name = "blockSuppressGrip",
				.offset = 0x27c38,
				.original{ 0x48, 0x83, 0x67, 0x08, 0xfb },
				.patched{ 0x48, 0x83, 0x67, 0x10, 0xfb } },
			Utils::BinaryPatcher::PatchTarget{
				.name = "forceSuppressGrip",
				.offset = 0x27c71,
				.original{ 0x48, 0x83, 0x4f, 0x08, 0x04 },
				.patched{ 0x48, 0x83, 0x4f, 0x10, 0x04 } }
		};

		Utils::BinaryPatcher g_patcher{
			kModuleName,
			"Compat::HIGGSUseTouchForGrip",
			std::span(kTargets)
		};
	}

	/// <summary>
	/// (Un)Applies the patches and returns the number of mismatches. ret 0 == Success.
	/// </summary>
	/// <param name="enabled"> Whether to apply or unapply the patch.</param>
	/// <returns></returns>
	void Apply(bool enabled)
	{
		// only for HIGGS 1.10.6
		g_patcher.Apply(enabled);
	}


	void Install()
	{
		if (g_patcher.ResolveTargets()) {
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
