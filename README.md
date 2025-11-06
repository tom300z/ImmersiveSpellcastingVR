# CommonLibSSE-NG Plugin Template

This is a basic plugin template using CommonLibSSE-NG.

### Requirements
* [XMake](https://xmake.io) [2.8.2+]
* C++23 Compiler (MSVC, Clang-CL)
* hkxvonv.exe in PATH
* Env var `XSE_TES5_MODS_PATH` set to ModOrganizer "mods" folder. The parent folder MUST contain `ModOrganizer.exe`.
* (For building MCM papyrus scripts) [SKSEVR](https://skse.silverlock.org/), [Skyrim VR Tools](https://www.nexusmods.com/skyrimspecialedition/mods/27782) & [SkyUI VR](https://www.nexusmods.com/skyrimspecialedition/mods/91535) installed in `%XSE_TES5_MODS_PATH%\..\ModOrganizer.exe`
* Env var `SkyrimVRPath` set to folder containing `SkyrimVR.exe`
* Creation Kit Installed to `SkyrimVRPath` (See [How do I set up creation kit for VR? ](https://www.reddit.com/r/skyrimvr/comments/1d5aloz/how_do_i_set_up_creation_kit_for_vr/))
* Skyrim papyrus scripts extracted to `%SkyrimVRPath%\data\scripts\source` (Open CreationKit once to do this)

## Getting Started
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

### Build Output (Optional)
If you want to redirect the build output, set one of or both of the following environment variables:

- Path to a Skyrim install folder: `XSE_TES5_GAME_PATH`

- Path to a Mod Manager mods folder: `XSE_TES5_MODS_PATH`

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

## Documentation
Please refer to the [Wiki](https://github.com/libxse/commonlibsse-ng-template/wiki/Home) for more advanced topics.
