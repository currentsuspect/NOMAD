@echo off
REM Build script for NOMAD DAW on Windows

echo Building NOMAD DAW...
echo.

REM Create build directory if it doesn't exist
if not exist build mkdir build

REM Navigate to build directory
cd build

REM Configure with CMake
echo Configuring project with CMake...
cmake .. -G "Visual Studio 17 2022"
if %errorlevel% neq 0 (
    echo CMake configuration failed!
    cd ..
    exit /b %errorlevel%
)

echo.
echo Configuration successful!
echo.
echo To build the project, run:
echo   cmake --build build --config Release
echo.
echo Or open build\NOMAD.sln in Visual Studio

cd ..
