#pragma once

#include "../../Core/NUITypes.h"
#include <Windows.h>
#include <functional>
#include <string>

namespace NomadUI {

/**
 * Windows x64 platform implementation for Nomad UI.
 * 
 * Handles:
 * - Window creation and management
 * - OpenGL context setup
 * - Event processing (mouse, keyboard, resize)
 * - High DPI support
 */
class NUIPlatformWindows {
public:
    NUIPlatformWindows();
    ~NUIPlatformWindows();
    
    // ========================================================================
    // Window Management
    // ========================================================================
    
    /**
     * Create a window with OpenGL context.
     */
    bool createWindow(int width, int height, const char* title);
    
    /**
     * Destroy the window and cleanup.
     */
    void destroyWindow();
    
    /**
     * Show the window.
     */
    void show();
    
    /**
     * Hide the window.
     */
    void hide();
    
    /**
     * Set window size.
     */
    void setSize(int width, int height);
    
    /**
     * Get window size.
     */
    void getSize(int& width, int& height) const;
    
    /**
     * Set window title.
     */
    void setTitle(const char* title);
    
    /**
     * Check if window should close.
     */
    bool shouldClose() const { return shouldClose_; }
    
    /**
     * Get native window handle.
     */
    HWND getHandle() const { return hwnd_; }
    
    // ========================================================================
    // OpenGL Context
    // ========================================================================
    
    /**
     * Make the OpenGL context current.
     */
    void makeContextCurrent();
    
    /**
     * Swap buffers (present frame).
     */
    void swapBuffers();
    
    /**
     * Set VSync enabled/disabled.
     */
    void setVSync(bool enabled);
    
    // ========================================================================
    // Event Processing
    // ========================================================================
    
    /**
     * Process pending window events.
     */
    void pollEvents();
    
    // ========================================================================
    // Callbacks
    // ========================================================================
    
    std::function<void(const NUIMouseEvent&)> onMouseEvent;
    std::function<void(const NUIKeyEvent&)> onKeyEvent;
    std::function<void(int, int)> onResize;
    std::function<void()> onClose;
    
private:
    // Window procedure
    static LRESULT CALLBACK WindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
    
    // Event conversion helpers
    NUIMouseEvent createMouseEvent(UINT msg, WPARAM wParam, LPARAM lParam);
    NUIKeyEvent createKeyEvent(UINT msg, WPARAM wParam, LPARAM lParam);
    NUIMouseButton getMouseButton(UINT msg);
    NUIKeyCode getKeyCode(WPARAM wParam);
    NUIModifiers getModifiers();
    
    // OpenGL setup
    bool setupOpenGL();
    bool setupPixelFormat();
    bool createGLContext();
    bool loadGLExtensions();
    
    // Window state
    HWND hwnd_ = nullptr;
    HDC hdc_ = nullptr;
    HGLRC hglrc_ = nullptr;
    HINSTANCE hInstance_ = nullptr;
    
    int width_ = 0;
    int height_ = 0;
    bool shouldClose_ = false;
    
    // Mouse state
    bool mouseTracking_ = false;
    NUIPoint lastMousePos_;
    
    // High DPI
    float dpiScale_ = 1.0f;
};

/**
 * Initialize the Windows platform (call once at startup).
 */
bool initializePlatform();

/**
 * Shutdown the Windows platform (call once at exit).
 */
void shutdownPlatform();

} // namespace NomadUI
