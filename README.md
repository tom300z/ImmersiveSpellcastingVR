# ISPVR - Immersive Spellcasting VR

Magic no longer feels like merely shooting a gun. Your physical hand position is now used as input for spellcasting, and you can feel the Magicka flowing through your hands thanks to a dynamic haptic system. Basically a much better version of VRIK's "Open Hand Casting".

## TLDR

* Cast spells immersively by opening and closing your hands!
* Get dynamic haptic feedback based on spell charge progress
* Allows jumping while holding a charged spell
* Removes transition animations that cause in-game animations to lag behind your physical hand
* Supports all controller types by allowing both grip_touch and grip_press as input methods
* Fully configurable via INI file or MCM
* Fully compatible with [Haptic Skyrim VR](https://www.nexusmods.com/skyrimspecialedition/mods/20364) (only replaces spellcasting haptics), [VRIK](https://www.nexusmods.com/skyrimspecialedition/mods/23416), [HIGGS](https://www.nexusmods.com/skyrimspecialedition/mods/43930), [Magic Improvements for Skyrim VR](https://www.nexusmods.com/skyrimspecialedition/mods/55751)
* *Advanced* Includes an optional patch for HIGGS 1.10.6 that makes it use grip_touch instead of grip_press for grabbing objects. This allows you to use grip_press for other inputs if you decide to use grip_touch for your hands.

## Installation

Use a mod manager, or manually put the files into your Data folder. Make sure to include the ESP in your `plugins.txt`.

### Requirements

* [SKSE VR](https://skse.silverlock.org/)
* [VR Address Library for SKSEVR](https://www.nexusmods.com/skyrimspecialedition/mods/58101)
* (optional for MCM) [SkyUI VR](https://www.nexusmods.com/skyrimspecialedition/mods/91535)

## Compatibility

The mod is incompatible with VRIK's "Open Hand Casting" as it replaces that feature. You have to disable it in the VRIK MCM (Controls → Cast Spells → NEVER).

Other than that, ISPVR was designed to be as compatible with other mods as possible. Compatibility with VRIK, HIGGS, Magic Improvements for Skyrim VR, and Haptic Skyrim VR has been tested.

If you discover a compatibility issue, please [report it](todo).

## How to use

### Fire & Forget / Charged spells (Fireball, Ice Spike, Fast Healing, Bound Weapons)

Close your hand to charge the spell. Once the haptic feedback spikes, the spell is charged. Aim and open your hand any time to release it.

> ***TIP:*** You can sheathe your weapons to abort a charged spell and reclaim your Magicka.

### Concentration spells (Flames, Sparks, Healing, Wards)

Open your hand to cast the spell, close it to stop.

But beware, with great power comes great responsibility! So don’t accidentally let go and shoot innocent bystanders (unless that’s your playstyle, of course).

> ***TIP:*** Sheathing your weapons can be helpful to avoid unwanted massacres.

## Configuration

You can configure the mod either through the INI file (located at `<SKYRIM_VR_INSTALL>\SKSE\Plugins\ImmersiveSpellcastingVR.ini`) or via the in-game MCM.
Both configuration methods are synced automatically.

## FAQ

<details>
<summary><b>Fire & Forget spells keep firing when I open my hand or start charging only after I open my hand.</b></summary>

You likely still have VRIK's "Open Hand Casting" enabled. This mod replaces that feature, so you must disable it in the VRIK MCM (Controls → Cast Spells → NEVER).

</details>

<details>
<summary><b>MCM not showing</b></summary>

New MCM menus sometimes take a while to show up, especially in existing saves. If it doesn't show up after a while, try running `setstage SKI_ConfigManagerInstance 1` in the console and wait for 5 minutes. Alternatively, use the INI file for configuration.

</details>

<details>
<summary><b>What is the difference between the `grip_touch` and `grip_press` input methods?</b></summary>

#### grip_touch

Triggered when you touch the grip button without significant force. On Valve Index controllers this correlates to closing your hands. On Oculus controllers this means touching the button without pressing it.

This input is not used by vanilla Skyrim VR at all, so using it avoids conflicts with vanilla inputs.

#### grip_press

Triggered when you apply force to the grip button. On Valve Index controllers this means squeezing the controller. On Oculus controllers this correlates to pressing down the grip button by closing your hand.

This input is used by vanilla Skyrim during gameplay for various actions. To avoid conflicts, you must remap the input using a tool like MoDErahN's [Key Remapping Tool](https://www.nexusmods.com/skyrimspecialedition/mods/68164). If this input method is used for ImmersiveSpellcastingVR, it will hide it from the game for every hand currently holding a drawn spell. You will be warned about this in-game.

</details>

<details>
<summary><b>I'm using the grip_touch input method but still trigger unwanted in-game actions when touching the grip button.</b></summary>

You probably have remapped the `grip_touch` button to emulate `grip_press` via a SteamVR Controller Binding.

To disable this:

* Open your SteamVR settings
* Go to "Controllers" → "SHOW BINDING UI"
* In the new window, click on "SHOW MORE BINDINGS" → "THE ELDER SCROLLS V: SKYRIM VR"
* Wait a few seconds for the bindings to load
* On the top left, under "Current Binding", click "Edit"
* On the left side, scroll down to the "Grip" section
* Under "Use as Button" it should say "Touch → Left Grip Button **Touch**". If it only says "Touch → Left Grip Button", click the pencil icon and change it to "Left Grip Button **Touch**"
* Repeat this on the right side of the screen
* On the bottom, click "Save Personal Binding" → "Save"

</details>

<details>
<summary><b>I think I’ve encountered a bug. Please help!</b></summary>

Please [open an issue](todo) on GitHub. I’ll try to fix it as soon as I can.

</details>

<details>
<summary><b>I have a question and the FAQ doesn’t answer it</b></summary>

Feel free to reach out via a post on the NexusMods mod page.

</details>

<details>
<summary><b>Is ImmersiveSpellcastingVR open source?</b></summary>

Yes, check it out on [GitHub](todo)!

</details>

## Special thanks

* Shizof for adding a compatibility API to HapticSkyrimVR quickly and providing helpful modding tips
* libxse for their [commonlibsse-ng-template](https://github.com/libxse/commonlibsse-ng-template)
* alandtse and contributors to commonlib for their extensive reverse engineering work and the [VR Address Library for SKSEVR](https://www.nexusmods.com/skyrimspecialedition/mods/58101)
* The SKSE team
* SARDONYX7 for [serde-hkx CLI (hkxc.exe)](https://www.nexusmods.com/skyrimspecialedition/mods/126214)
* hexabit for the [Bethesda Archive Extractor](https://www.nexusmods.com/skyrimspecialedition/mods/974)
* The NSA for [Ghidra](https://github.com/NationalSecurityAgency/ghidra), I guess.

## Developer Guide

**The following guide is only relevant for contributors. If you want to use this mod, see [Installation](#installation) and [FAQ](#faq).**

I’ve tried to make this as easy to set up as possible, but I may have missed some details, so feel free to open an issue.

### Requirements

* [XMake](https://xmake.io) 2.8.2+
* C++23 compiler (MSVC or Clang-CL)
* [serde-hkx CLI (hkxc.exe)](https://www.nexusmods.com/skyrimspecialedition/mods/126214) in your PATH
* Env var `XSE_TES5_MODS_PATH` set to the ModOrganizer "mods" folder. The parent folder must contain `ModOrganizer.exe`.
* (For building MCM Papyrus scripts) [SKSEVR](https://skse.silverlock.org/), [VR Address Library for SKSEVR](https://www.nexusmods.com/skyrimspecialedition/mods/58101), [Skyrim VR Tools](https://www.nexusmods.com/skyrimspecialedition/mods/27782), and [SkyUI VR](https://www.nexusmods.com/skyrimspecialedition/mods/91535) installed in `%XSE_TES5_MODS_PATH%\..\ModOrganizer.exe`
* Env var `SkyrimVRPath` set to the folder containing `SkyrimVR.exe`
* Creation Kit installed to `SkyrimVRPath` (see [How do I set up Creation Kit for VR?](https://www.reddit.com/r/skyrimvr/comments/1d5aloz/how_do_i_set_up_creation_kit_for_vr/))
* Skyrim Papyrus scripts extracted to `%SkyrimVRPath%\data\scripts\source` (open Creation Kit once to do this)

### Getting Started

```bat
git clone --recurse-submodules <TODO: add repo URL>
cd ImmersiveSpellcastingVR
```

### Build

```bat
xmake build
```

> ***Note:*** This will generate a `build/windows/` directory in the project’s root directory with the build output. It will also install the mod to `XSE_TES5_MODS_PATH` (must be enabled manually in MO2).

When building in debug mode, Skyrim VR will automatically be (re)started through ModOrganizer to ensure the current mod version is available for debugging. To make experimenting more efficient, I recommend powerofthree’s [Start On Save](https://www.nexusmods.com/skyrimspecialedition/mods/50054) mod.

### Switching build modes

```bat
xmake f -m debug
xmake f -m releasedbg
```

### Project Generation (Optional)

```bat
xmake project -k vsxmake
```

> ***Note:*** This will generate a `vsxmakeXXXX/` directory in the project’s root directory using the latest version of Visual Studio installed on the system.

#### Debugging

To configure Visual Studio to attach its debugger to SkyrimVR after build:

1. Right-click `ImmersiveSpellcastingVR` in the Solution Explorer.
2. Go to `Properties` → `Configuration Properties` → `Debugging`.
3. Set `Command` to `SkyrimVR.exe` and `Attach` to `Yes`.

Clicking the Debug button should now automatically stop Skyrim, build and install the mod, start Skyrim, and attach the debugger to it.

> ***Note:*** This has to be repeated every time the VS project is regenerated.

### Upgrading Packages (Optional)

```bat
xmake repo --update
xmake require --upgrade
```
