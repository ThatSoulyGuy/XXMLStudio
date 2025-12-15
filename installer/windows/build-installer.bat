@echo off
REM Windows Installer Build Script for XXMLStudio
REM This is a wrapper that calls the PowerShell script

setlocal

REM Check for PowerShell
where powershell >nul 2>&1
if %ERRORLEVEL% neq 0 (
    echo ERROR: PowerShell is required to run this script
    exit /b 1
)

REM Get script directory
set "SCRIPT_DIR=%~dp0"

REM Run PowerShell script with all arguments passed through
powershell -ExecutionPolicy Bypass -File "%SCRIPT_DIR%build-installer.ps1" %*

endlocal
