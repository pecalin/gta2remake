@echo off
echo ========================================
echo   GTA2 Clone - Windows Build Script
echo ========================================
echo.

:: Check for CMake
where cmake >nul 2>nul
if %ERRORLEVEL% NEQ 0 (
    echo ERROR: cmake not found in PATH!
    echo Install CMake and make sure "Add to PATH" is checked.
    pause
    exit /b 1
)

:: Check for g++
where g++ >nul 2>nul
if %ERRORLEVEL% NEQ 0 (
    echo ERROR: g++ not found in PATH!
    echo.
    echo Install MinGW-w64:
    echo   1. Download from: https://github.com/niXman/mingw-builds-binaries/releases
    echo   2. Extract to C:\mingw64
    echo   3. Add C:\mingw64\bin to your system PATH
    echo   4. Restart this command prompt and try again
    pause
    exit /b 1
)

echo Found CMake:
cmake --version | findstr /C:"version"
echo.
echo Found g++:
g++ --version | findstr /C:"g++"
echo.

:: Create build directory
if not exist build_win mkdir build_win
cd build_win

:: Configure with CMake using MinGW Makefiles
echo [1/3] Configuring with CMake (MinGW)...
cmake .. -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Release
if %ERRORLEVEL% NEQ 0 (
    echo.
    echo CMake configuration failed!
    pause
    exit /b 1
)

:: Build
echo.
echo [2/3] Building...
cmake --build . -j%NUMBER_OF_PROCESSORS%
if %ERRORLEVEL% NEQ 0 (
    echo.
    echo Build failed!
    pause
    exit /b 1
)

echo.
echo ========================================
echo   BUILD SUCCESSFUL!
echo ========================================
echo.
echo Game executable: build_win\gta2.exe
echo.
echo Controls:
echo   WASD/Arrows  - Move / Drive
echo   F            - Enter/Exit vehicle
echo   Space        - Handbrake (drift)
echo   Left Ctrl    - Shoot
echo   Q / E        - Switch weapon
echo.
echo Starting game...
echo.
start gta2.exe
