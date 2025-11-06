@echo off
setlocal

rem --- configuration ---
set "MO2Path=%XSE_TES5_MODS_PATH%\.."
set "SCRIPT_DIR=%~dp0"
rem ----------------------

if "%~1"=="" (
    echo Usage: %~nx0 ^<installpath^>
    exit /b 1
)

set "CMD_PATH=%SystemRoot%\System32\cmd.exe"
set "CMD_ARGS=/c \"\"%SCRIPT_DIR%compile_papyrus.cmd\" \"%~1\"\""

"%MO2Path%\ModOrganizer.exe" --multiple --logs run ^
  -a "%CMD_ARGS%" ^
  "%CMD_PATH%"
endlocal
