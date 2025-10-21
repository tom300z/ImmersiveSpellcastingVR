mkdir %MO2Path%\overwrite\scripts
set "SCRIPT_DIR=%~dp0"
%MO2Path%\ModOrganizer.exe --multiple --logs run "%SCRIPT_DIR%compile_papyrus.bat"