#include "NUIDPIHelper.h"
#include <iostream>

// For DPI awareness APIs
#include <ShellScalingApi.h>
#pragma comment(lib, "Shcore.lib")

namespace NomadUI {

bool NUIDPIHelper::initializeDPI() {
    // Try to set per-monitor DPI awareness (Windows 10 1703+)
    typedef BOOL(WINAPI* SetProcessDpiAwarenessContextProc)(DPI_AWARENESS_CONTEXT);
    
    HMODULE user32 = LoadLibraryA("user32.dll");
    if (user32) {
        auto setProcessDpiAwarenessContext = 
            (SetProcessDpiAwarenessContextProc)GetProcAddress(user32, "SetProcessDpiAwarenessContext");
        
        if (setProcessDpiAwarenessContext) {
            // Try per-monitor V2 (best)
            if (setProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2)) {
                std::cout << "DPI: Per-Monitor V2 awareness enabled" << std::endl;
                FreeLibrary(user32);
                return true;
            }
            
            // Fall back to per-monitor V1
            if (setProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE)) {
                std::cout << "DPI: Per-Monitor V1 awareness enabled" << std::endl;
                FreeLibrary(user32);
                return true;
            }
        }
        FreeLibrary(user32);
    }
    
    // Try SetProcessDpiAwareness (Windows 8.1+)
    HRESULT hr = SetProcessDpiAwareness(PROCESS_PER_MONITOR_DPI_AWARE);
    if (SUCCEEDED(hr)) {
        std::cout << "DPI: Per-Monitor awareness enabled (8.1)" << std::endl;
        return true;
    }
    
    // Fall back to SetProcessDPIAware (Windows Vista+)
    if (SetProcessDPIAware()) {
        std::cout << "DPI: System awareness enabled (Vista)" << std::endl;
        return true;
    }
    
    std::cerr << "DPI: Failed to set DPI awareness" << std::endl;
    return false;
}

float NUIDPIHelper::getDPIScale(HWND hwnd) {
    int dpi = getDPI(hwnd);
    return dpi / 96.0f;  // 96 DPI = 100% scaling
}

int NUIDPIHelper::getDPI(HWND hwnd) {
    // Try GetDpiForWindow (Windows 10 1607+)
    typedef UINT(WINAPI* GetDpiForWindowProc)(HWND);
    
    HMODULE user32 = LoadLibraryA("user32.dll");
    if (user32) {
        auto getDpiForWindow = (GetDpiForWindowProc)GetProcAddress(user32, "GetDpiForWindow");
        if (getDpiForWindow) {
            UINT dpi = getDpiForWindow(hwnd);
            FreeLibrary(user32);
            if (dpi > 0) {
                return static_cast<int>(dpi);
            }
        }
        FreeLibrary(user32);
    }
    
    // Fall back to GetDpiForMonitor (Windows 8.1+)
    if (hwnd) {
        HMONITOR monitor = MonitorFromWindow(hwnd, MONITOR_DEFAULTTONEAREST);
        if (monitor) {
            UINT dpiX = 96, dpiY = 96;
            if (SUCCEEDED(GetDpiForMonitor(monitor, MDT_EFFECTIVE_DPI, &dpiX, &dpiY))) {
                return static_cast<int>(dpiX);
            }
        }
    }
    
    // Fall back to system DPI
    HDC hdc = GetDC(nullptr);
    if (hdc) {
        int dpi = GetDeviceCaps(hdc, LOGPIXELSX);
        ReleaseDC(nullptr, hdc);
        return dpi;
    }
    
    // Default to 96 DPI (100% scaling)
    return 96;
}

int NUIDPIHelper::scaleToDPI(int value, float dpiScale) {
    return static_cast<int>(value * dpiScale + 0.5f);  // Round to nearest
}

int NUIDPIHelper::scaleFromDPI(int value, float dpiScale) {
    return static_cast<int>(value / dpiScale + 0.5f);  // Round to nearest
}

} // namespace NomadUI
