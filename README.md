# ImmersiveSpellcastingVR

## TLDR
- Cast spells immersively by opening and closing your hands!
- Get dynamic haptic feedback based on spell charge progress
- Allows jumping while holding a charged spell
- Removes transision animations that cause spell animations to behind your physical finger position
- Supports all controller types by allowing both grip_touch and grip_press as input methods
- Fully configurable via ini file or MCM
- Fully compatible with [Haptic Skyrim VR](https://www.nexusmods.com/skyrimspecialedition/mods/20364) (only replaces spellcasting haptics), [VRIK](https://www.nexusmods.com/skyrimspecialedition/mods/23416), [HIGGS](https://www.nexusmods.com/skyrimspecialedition/mods/43930), [Magic Improvements for Skyrim VR](https://www.nexusmods.com/skyrimspecialedition/mods/55751)

## Installation
todo
### Dependencies
todo

## How to use
Magic longer feels like merely shooting a gun. Your physical hand position is now used as the input for spellcasting. Basically a better version of VRIK's "Open Hand Casting".

### Fire & forget / Charged spells (Fireball, Ice Spike, Fast Healing, Bound weapons)
Close your hand to charge the spell. Once the haptic feedback spikes, the spell is charged. Aim and open your hand any time to release it.

> ***TIP:*** You can sheathe your weapons to abort a charged spell and reclaim your Magicka

### Concentration spells (Flames, Sparks, Healing, Wards)
Open your hand to cast the spell, close it to stop.

But beware, with great power comes great responsibility! So don't acidentally let go and shoot innocent bystanders (unless thats your playstyle of course xD).
> ***TIP:*** Sheathing your weapons can be helpful to avoid unwanted massacres

## Configuration
You can configure the mod either through the INI file located at `%USERPROFILE%\Documents\My Games\SkyrimVR`

## FAQ
### MCM not showing
New MCM menus sometimes take a while to show up, especially in existing saves. If it doesn't show up after a while, try running `setstage SKI_ConfigManagerInstance 1` in the console and wait for 5 minutes. Alternatively use the INI file for configuration.


## Special thanks
- Shizof for adding a compatibility API to HapticSkyrimVR super quickly and giving me a few modding tips
- libxse for their [commonlibsse-ng-template](https://github.com/libxse/commonlibsse-ng-template)
- alandtse and all the other contributors to commonlib for their extensive reverse engineering efforts as well as the [VR Address Library for SKSEVR](https://www.nexusmods.com/skyrimspecialedition/mods/58101)
- the SKSE team
- SARDONYX7 for [serde-hkx CLI (hkxc.exe)](https://www.nexusmods.com/skyrimspecialedition/mods/126214)
- hexabit for the [Bethesda Archive Extractor](https://www.nexusmods.com/skyrimspecialedition/mods/974)
- the NSA for [Ghidra](https://github.com/NationalSecurityAgency/ghidra), i guess...


## Developer Guide
**The following guide is only relevant for contributors. If you want to use this mod see [Installation](#Installation) & [FAQ](FAQ)**

I've tried to make this as easy-to-set-up as possible, but i probably forgot to mention a few minor things so feel free to open an Issue

### Requirements
* [XMake](https://xmake.io) [2.8.2+]
* C++23 Compiler (MSVC, Clang-CL)
* [serde-hkx CLI (hkxc.exe)](https://www.nexusmods.com/skyrimspecialedition/mods/126214) in your PATH
* Env var `XSE_TES5_MODS_PATH` set to ModOrganizer "mods" folder. The parent folder MUST contain `ModOrganizer.exe`.
* (For building MCM papyrus scripts) [SKSEVR](https://skse.silverlock.org/), [VR Address Library for SKSEVR](https://www.nexusmods.com/skyrimspecialedition/mods/58101), [Skyrim VR Tools](https://www.nexusmods.com/skyrimspecialedition/mods/27782) & [SkyUI VR](https://www.nexusmods.com/skyrimspecialedition/mods/91535) installed in `%XSE_TES5_MODS_PATH%\..\ModOrganizer.exe`
* Env var `SkyrimVRPath` set to folder containing `SkyrimVR.exe`
* Creation Kit Installed to `SkyrimVRPath` (See [How do I set up creation kit for VR? ](https://www.reddit.com/r/skyrimvr/comments/1d5aloz/how_do_i_set_up_creation_kit_for_vr/))
* Skyrim papyrus scripts extracted to `%SkyrimVRPath%\data\scripts\source` (Open CreationKit once to do this)

### Getting Started
```bat
git clone --recurse-submodules <TOOD: add repo url>
cd ImmersiveSpellcastingVR
```

### Build
To build the project, run the following command:
```bat
xmake build
```

> ***Note:*** *This will generate a `build/windows/` directory in the **project's root directory** with the build output. It will also install the mod to `XSE_TES5_MODS_PATH` (must be enabled manually in MO2).*

When in building in debug mode skyrim vr will automatically be (re)started thorugh ModOrganizer to ensure the currend mod version is available for debugging. To to make experimenting even more efficient, i reccommend powerofthree's [Start On Save](https://www.nexusmods.com/skyrimspecialedition/mods/50054) mod

### Switching build modes
```bat
xmake f -m debug
xmake f -m releasedbg
```

### Project Generation (Optional)
If you want to generate a Visual Studio project, run the following command:
```bat
xmake project -k vsxmake
```

> ***Note:*** *This will generate a `vsxmakeXXXX/` directory in the **project's root directory** using the latest version of Visual Studio installed on the system.*

### Upgrading Packages (Optional)
If you want to upgrade the project's dependencies, run the following commands:
```bat
xmake repo --update
xmake require --upgrade
```
