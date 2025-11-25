#include "Settings.h"

#include <array>
#include <string>

#include "utils/Input.h"

namespace Settings
{
	namespace
	{
		Config::Value DefaultInputMethod()
		{
			return Config::Value{ Utils::Input::IsUsingIndexControllers() ? std::string("grip_touch") : std::string("grip_press") };
		}

		const std::array<Config::SettingDefinition, 6> kDefinitions = {
			Config::SettingDefinition{ kInputMethod, Config::Type::kString, Config::Value{ std::string("grip_press") }, "OpenVR button name that should be treated as the casting button. Options: 'grip_touch' (recommended for index controllers), 'grip_press' (recommended for oculus)", "Input", &DefaultInputMethod },
			Config::SettingDefinition{ kInputShowBindingWarning, Config::Type::kBool, Config::Value{ true }, "Show a warning when the grip button is bound in the gameplay context.", "Input", nullptr },
			Config::SettingDefinition{ kInputEnable, Config::Type::kBool, Config::Value{ true }, "Enable Immersive Casting VR's input redirection system.", "Input", nullptr },
			Config::SettingDefinition{ kInputCastAfterMenuExit, Config::Type::kBool, Config::Value{ true }, "Immediately resumes casting after closing menus if the hand is in casting position. If disabled hands have to be closed/opened once after leaving a menu.", "Input", nullptr },
			Config::SettingDefinition{ kInputHackHiggsTouchInput, Config::Type::kBool, Config::Value{ false }, "Hacks HIGGS to make it use grip_touch instead of grip_press for grabbing stuff. This way you can use grip_press for other inputs.", "Input", nullptr },
			Config::SettingDefinition{ kHapticsEnable, Config::Type::kBool, Config::Value{ true }, "Enable Immersive Casting VR's spellcasting haptics integration. Disables other mod's spellcasting haptics (such as HapticSkyrimVR).", "Haptics", nullptr },
		};
	}

	std::span<const Config::SettingDefinition> GetSettingDefinitions()
	{
		return kDefinitions;
	}
}
