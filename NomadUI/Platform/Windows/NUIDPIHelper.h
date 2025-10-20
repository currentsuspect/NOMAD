#pragma once

#include <Windows.h>

namespace NomadUI {

/**
 * Helper for handling High DPI on Windows.
 * Call initializeDPI() at application startup before creating any windows.
 */
class NUIDPIHelper {
public:
    /**
     * Initialize DPI awareness for the application.
     * Must be called before creating any windows.
     */
    static bool initializeDPI();
    
    /**
     * Get DPI scale factor for a window.
     * Returns 1.0 for 96 DPI (100% scaling)
     * Returns 1.5 for 144 DPI (150% scaling)
     * Returns 2.0 for 192 DPI (200% scaling)
     */
    static float getDPIScale(HWND hwnd);
    
    /**
     * Get DPI for a window.
     * Returns 96 for 100% scaling, 144 for 150%, 192 for 200%, etc.
     */
    static int getDPI(HWND hwnd);
    
    /**
     * Scale a value by DPI.
     */
    static int scaleToDPI(int value, float dpiScale);
    
    /**
     * Scale a value from DPI.
     */
    static int scaleFromDPI(int value, float dpiScale);
};

} // namespace NomadUI
