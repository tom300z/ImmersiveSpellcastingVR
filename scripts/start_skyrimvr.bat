powershell start-process -FilePath %MO2Path%\ModOrganizer.exe -ArgumentList "--multiple,%SkyrimVRPath%\sksevr_loader.exe,-forcesteamloader"

:search
tasklist|find "SkyrimVR"
IF %ERRORLEVEL% == 0 GOTO end
ping -n 1 /w 50 127.0.0.1 >NUL
GOTO search

:end