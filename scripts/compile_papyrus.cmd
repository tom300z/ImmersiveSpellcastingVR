@echo off
setlocal

set "installdir=%~1"
set "SCRIPT_DIR=%~dp0"

rem Ensure target directories exist
mkdir "%installdir%\scripts" 2>nul

rem Recreate ImmersiveCasting source folder
rmdir /s /q "%SkyrimVRPath%\data\scripts\source\ImmersiveCasting" 2>nul
mkdir "%SkyrimVRPath%\data\scripts\source\ImmersiveCasting"

rem Copy source files
xcopy /y "%SCRIPT_DIR%..\mod_data\scripts\*.*" "%SkyrimVRPath%\data\scripts\source\ImmersiveCasting\" >nul
xcopy /y "%SCRIPT_DIR%..\mod_data\scripts\*.*" "%SkyrimVRPath%\data\scripts\source\" >nul

rem Compile Papyrus scripts
"%PapyrusCompilerPath%\PapyrusCompiler.exe" ^
  "%SkyrimVRPath%\data\scripts\source\ImmersiveCasting" ^
  -all ^
  -f=TESV_Papyrus_Flags.flg ^
  -i="%SkyrimVRPath%\data\scripts\source" ^
  -o="%installdir%\scripts"

endlocal
