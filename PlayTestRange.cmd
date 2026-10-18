@echo off
rem Developer test range: cover, targets and enemies without playing the intro.
powershell.exe -NoProfile -ExecutionPolicy Bypass -File "%~dp0Scripts\VFTestRange.ps1" Play
