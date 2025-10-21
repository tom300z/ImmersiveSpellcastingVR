mkdir %MO2Path%\overwrite\scripts
set "SCRIPT_DIR=%~dp0"
rmdir /s /q "%SkyrimVRPath%\data\scripts\source\ImmersiveCasting"
mkdir "%SkyrimVRPath%\data\scripts\source\ImmersiveCasting"
xcopy /y "%SCRIPT_DIR%..\papyrus\*.*" "%SkyrimVRPath%\data\scripts\source\ImmersiveCasting\"
xcopy /y "%SCRIPT_DIR%..\papyrus\*.*" "%SkyrimVRPath%\data\scripts\source\"
"%PapyrusCompilerPath%\PapyrusCompiler.exe" "%SkyrimVRPath%\data\scripts\source\ImmersiveCasting" -all -f=TESV_Papyrus_Flags.flg -i="%SkyrimVRPath%\data\scripts\source" -o=%MO2Path%\overwrite\scripts\