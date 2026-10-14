@echo off
rem Optional: PlayIntro.cmd HOLD_SHOP  (start at a mission state)
if "%~1"=="" (powershell.exe -NoProfile -ExecutionPolicy Bypass -File "%~dp0Scripts\VFIntro.ps1" Play) else (powershell.exe -NoProfile -ExecutionPolicy Bypass -File "%~dp0Scripts\VFIntro.ps1" Play -State %1)
