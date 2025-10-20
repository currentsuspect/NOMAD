@echo off
echo ========================================
echo   Building NomadUI Slider & Text Demo
echo ========================================
echo.

REM Create build directory
if not exist build mkdir build
cd build

REM Configure with CMake
echo Configuring with CMake...
cmake .. -G "Visual Studio 17 2022" -A x64 -DNOMADUI_BUILD_EXAMPLES=ON -DNOMADUI_BUILD_OPENGL=ON

if %ERRORLEVEL% neq 0 (
    echo ERROR: CMake configuration failed!
    pause
    exit /b 1
)

REM Build the demo
echo.
echo Building SliderTextDemo...
cmake --build . --config Release --target NomadUI_SliderTextDemo

if %ERRORLEVEL% neq 0 (
    echo ERROR: Build failed!
    pause
    exit /b 1
)

echo.
echo ========================================
echo   Build successful!
echo ========================================
echo.
echo Running the demo...
echo.

REM Run the demo
bin\Release\NomadUI_SliderTextDemo.exe

echo.
echo Demo finished.
pause
