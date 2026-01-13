@echo off
setlocal

echo ==========================================
echo MameCloudRom Build Script
echo ==========================================

:: 1. Check for WinFsp
set "WINFSP_INC=C:\Program Files (x86)\WinFsp\inc\winfsp\winfsp.h"

if exist "%WINFSP_INC%" goto found_winfsp

echo [ERROR] WinFsp headers not found at:
echo %WINFSP_INC%
echo.
echo Please install WinFsp from https://winfsp.dev/
echo IMPORTANT: Make sure to install "Core" and "Developer" components.
echo.
pause
exit /b 1

:found_winfsp

:: 2. Check CMake
where cmake >nul 2>nul
if %errorlevel% neq 0 (
    echo [ERROR] CMake not found in PATH.
    echo Please open this script from a "Visual Studio Developer Command Prompt"
    echo or ensure CMake is installed and in your PATH.
    pause
    exit /b 1
)

:: 3. Create build directory
if not exist build (
    echo [INFO] Creating build directory...
    mkdir build
)

:: 4. Run CMake Generation
cd build
echo [INFO] Running CMake configuration...
cmake ..
if %errorlevel% neq 0 (
    echo.
    echo [ERROR] CMake configuration failed.
    echo Please ensure Visual Studio 2022 C++ tools are installed.
    cd ..
    pause
    exit /b 1
)

:: 5. Run Build
echo [INFO] Building Release configuration...
cmake --build . --config Release
if %errorlevel% neq 0 (
    echo.
    echo [ERROR] Build failed.
    cd ..
    pause
    exit /b 1
)

echo [INFO] Copying binaries to the project directory...
copy /Y "Release\mcr.exe" "..\" >nul 2>nul
if %errorlevel% neq 0 (
    echo [WARNING] Failed to copy mcr.exe. It might be running.
    echo [INFO] Attempting to stop existing mcr.exe process...
    
    :: Try to kill the process loop
    for /L %%i in (1,1,3) do (
        taskkill /F /IM mcr.exe >nul 2>nul
        timeout /t 2 /nobreak >nul
        copy /Y "Release\mcr.exe" "..\" >nul 2>nul
        if not errorlevel 1 goto copy_success
        echo [RETRY %%i] Waiting for file release...
    )

    :: Final attempt
    taskkill /F /IM mcr.exe >nul 2>nul
    timeout /t 3 /nobreak >nul
    copy /Y "Release\mcr.exe" "..\" >nul
    
    if %errorlevel% neq 0 (
        echo [ERROR] Still failed to copy mcr.exe after multiple attempts.
        echo Please close it manually via Task Manager and try again.
        cd ..
        pause
        exit /b 1
    )
)

:copy_success
echo [INFO] Successfully updated mcr.exe.

copy /Y "Release\winfsp-x64.dll" "..\" >nul 2>nul

echo ==========================================
echo [SUCCESS] Build completed successfully!
echo Executable location: mcr.exe (Project Root)
echo ==========================================
echo.
cd ..
pause
