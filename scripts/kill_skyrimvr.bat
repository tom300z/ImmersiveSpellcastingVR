@echo off
powershell.exe -NoProfile -ExecutionPolicy Bypass -Command "& { $proc = Get-Process -Name 'SkyrimVR' -ErrorAction SilentlyContinue; if ($proc) { $proc | Stop-Process -Force } }"
