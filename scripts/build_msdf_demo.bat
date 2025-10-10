@echo off
echo Building MSDF Text Demo...

REM Create build directory
if not exist build mkdir build
cd build

REM Configure with CMake
cmake .. -G "Visual Studio 17 2022" -A x64
if %ERRORLEVEL% neq 0 (
    echo CMake configuration failed
    exit /b 1
)

REM Build the demo
cmake --build . --config Release --target TEXT_DEMO
if %ERRORLEVEL% neq 0 (
    echo Build failed
    exit /b 1
)

echo Build completed successfully!
echo Run: build\Release\TEXT_DEMO.exe