# ISPVR - Immersive Spellcasting VR
Magic no longer feels like merely shooting a gun. Your physical hand position is now used as input for spellcasting, and you can feel the Magicka flowing through your hands thanks to a dynamic haptic system. Basically a much better version of VRIK's "Open Hand Casting".

## TLDR
- Cast spells immersively by opening and closing your hands!
- Get dynamic haptic feedback based on spell charge progress
- Allows jumping while holding a charged spell
- Removes transition animations that cause in-game animations to lag behind your physical hand
- Supports all controller types by allowing both grip_touch and grip_press as input methods
- Fully configurable via ini file or MCM
- Fully compatible with [Haptic Skyrim VR](https://www.nexusmods.com/skyrimspecialedition/mods/20364) (only replaces spellcasting haptics), [VRIK](https://www.nexusmods.com/skyrimspecialedition/mods/23416), [HIGGS](https://www.nexusmods.com/skyrimspecialedition/mods/43930), [Magic Improvements for Skyrim VR](https://www.nexusmods.com/skyrimspecialedition/mods/55751)
- *Advanced* Includes an optional patch for HIGGS 1.10.6 that makes it use grip_touch instead of grip_press for grabbing objects. This allows you to use grip_press for other inputs if you decide to use grip_touch for your hands.

## Installation
Use a mod manager, or manually put the files into your Data folder. Make sure to include the esp in your plugins.txt

### Requirements
- [SKSE VR](https://skse.silverlock.org/)
- [VR Address Library for SKSEVR](https://www.nexusmods.com/skyrimspecialedition/mods/58101)
- (optional for MCM) [SkyUI VR](https://www.nexusmods.com/skyrimspecialedition/mods/91535)

## Compatibility
The mod is incompatible with VRIK's "Open Hand Casting" as it replaces that feature. You have to disable it in the VRIK MCM (Controls, Cast Spells > NEVER).

Other than that, ISPVR was designed to be as compatible with other mods as possible. Compatibility with VRIK, HIGGS, Magic Improvements for Skyrim VR & Haptic Skyrim VR has been tested.

If you discover a compatibility issue please [report it](todo).

## How to use

### Fire & forget / Charged spells (Fireball, Ice Spike, Fast Healing, Bound weapons)
Close your hand to charge the spell. Once the haptic feedback spikes, the spell is charged. Aim and open your hand any time to release it.

> ***TIP:*** You can sheathe your weapons to abort a charged spell and reclaim your Magicka

### Concentration spells (Flames, Sparks, Healing, Wards)
Open your hand to cast the spell, close it to stop.

But beware, with great power comes great responsibility! So don't acidentally let go and shoot innocent bystanders (unless thats your playstyle of course).
> ***TIP:*** Sheathing your weapons can be helpful to avoid unwanted massacres

## Configuration
You can configure the mod either through the INI file (located at `<SKYRIM_VR_INSTALL>\SKSE\Plugins\ImmersiveSpellcastingVR.ini`) or via the ingame MCM.
Both configuration methods are synced automatically.

## FAQ
<details>
<summary> <b>Fire & Forget spells keep firing when i open my Hand/start charging only after i open my hand.</b> </summary>

You probably have VRIK's "Open Hand Casting" enabled. This mod replaces that feature so you have to disable it in the VRIK MCM (Controls, Cast Spells > NEVER)
</details>

<details>
<summary> <b>MCM not showing</b> </summary>

New MCM menus sometimes take a while to show up, especially in existing saves. If it doesn't show up after a while, try running `setstage SKI_ConfigManagerInstance 1` in the console and wait for 5 minutes. Alternatively use the INI file for configuration.
</details>

<details>
<summary><b>What is the difference between the `grip_touch` and `grip_press` input methods?</b></summary>

#### grip_touch
Triggered when you touch the grip button without a significant force. On Valve Index Controllers this correlates to closing your hands. On Oculus Controllers this mean touching the button without pressing it.

This input is not used by Vanilla Skyrim VR at all, so using it avoids conflicts with Vanilla inputs.

#### grip_press
Triggered when you apply a force to the grip button. On Valve Index controllers this means squeezing the controller. On Oculus controllers this correlates to pressing down the grip button by closing your hand.

This input is used by vanilla Skyrim during gameplay for various actions. To avoid conflicts you must remap the input in gameplay by using a tool like MoDErahN's [Key Remapping Tool](https://www.nexusmods.com/skyrimspecialedition/mods/68164). If this input method is used for ImmersiveSpellcastingVR, it will hide it from the game for every hand that is currently holding a drawn spell. You will be warned about this in-game.
</details>

<details>
<summary> <b>I'm using the grip_touch input method but i still trigger unwanted ingame actions when touching the grip button</b> </summary>

You probably have remapped the `grip_touch` button to emulate `grip_press` via the SteamVR Controller Binding UI.

To disable this:

- Open your SteamVR settings
- Go to "Controllers" > "SHOW BINDING UI"
- In the new window: Click on "SHOW MORE BINDINGS" > "THE ELDER SCROLLS V: SKYRIM VR"
- Wait a few settings for the bindings to load
- On the top left, under "Current Binding", click the "Edit" button
- On the left side, scroll down to the "Grip" section
- Under "Use as Button" it should say "Touch > Left Grip Button **Touch**". If it only says "Touch > Left Grip Button", click the pencil icon and change it to "Left Grip Button **Touch**"
- Repeat this for the right side
- On the bottom, click "Save Personal Binding" > "Save"
</details>

<details>
<summary> <b>I think i've encountered a bug. Please help!</b> </summary>

Please [open an Issue](todo) on Github. I'll try to fix it as soon as i can.
</details>

<details>
<summary> <b>I have a question and the FAQ doesn't answer it</b> </summary>

Feel free to reach out via a post on the Nexusmods mod page
</details>

<details>
<summary> <b>Is the ImmersiveSpellcastingVR Open Source</b> </summary>

Yes, check it out on [Github](todo)!
</details>

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

#### Debugging
To configure Visual Studio to attach it's debugger to SkyrimVR after build, right click `ImmersiveSpellcastingVR` in the Solution Explorer, go to `Properties`, `Configuration Properties`, `Debugging`. Set `Command` to `SkyrimVR.exe` and `Attach` to `Yes`.

Clicking the Debug button should now automatically: Stop Skyrim, build and install the mod, Start skyrim and attach the debugger to it.

> ***Note:*** This has to be repeated every time the vsproject is regenerated


### Upgrading Packages (Optional)
If you want to upgrade the project's dependencies, run the following commands:
```bat
xmake repo --update
xmake require --upgrade
```
