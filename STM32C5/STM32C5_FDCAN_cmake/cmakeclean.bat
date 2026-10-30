@echo off
setlocal
if "%~1"=="" (
    powershell.exe -NoLogo -NoProfile -ExecutionPolicy Bypass -File "%~dp0clean_cmake.ps1" -Clean -KeepFirmware
) else if /I "%~1"=="-Preview" (
    powershell.exe -NoLogo -NoProfile -ExecutionPolicy Bypass -File "%~dp0clean_cmake.ps1" -KeepFirmware
) else (
    powershell.exe -NoLogo -NoProfile -ExecutionPolicy Bypass -File "%~dp0clean_cmake.ps1" %*
)
set "clean_result=%errorlevel%"
if "%~1"=="" pause
exit /b %clean_result%
