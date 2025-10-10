#pragma once

#include "../../Core/NUITypes.h"
#include <string>
#include <functional>
#include <cstdint>

// Opaque handle types (platform-specific, defined in .cpp)
struct NUIWindowHandleWin32;
typedef NUIWindowHandleWin32* NUIWindowHandle;

namespace NomadUI {

// Forward declarations
class NUIRenderer;
class NUIComponent;

/**
 * Windows-specific window implementation
 * Handles Win32 window creation, OpenGL context, and event handling
 */
class NUIWindowWin32 {
public:
    NUIWindowWin32();
    ~NUIWindowWin32();

    // Window creation and management
    bool create(const std::string& title, int width, int height);
    void destroy();
    void show();
    void hide();
    
    // Main loop
    bool processEvents();  // Returns false when window should close
    void swapBuffers();
    
    // Window properties
    void setTitle(const std::string& title);
    void setSize(int width, int height);
    void getSize(int& width, int& height) const;
    void setPosition(int x, int y);
    
    // OpenGL context
    bool createGLContext();
    bool makeContextCurrent();
    
    // Event callbacks
    void setMouseMoveCallback(std::function<void(int, int)> callback);
    void setMouseButtonCallback(std::function<void(int, bool)> callback);
    void setMouseWheelCallback(std::function<void(float)> callback);
    void setKeyCallback(std::function<void(int, bool)> callback);
    void setResizeCallback(std::function<void(int, int)> callback);
    void setCloseCallback(std::function<void()> callback);
    
    // Root component
    void setRootComponent(NUIComponent* root);
    NUIComponent* getRootComponent() const { return m_rootComponent; }
    
    // Renderer
    void setRenderer(NUIRenderer* renderer);
    NUIRenderer* getRenderer() const { return m_renderer; }
    
    // Native handles (void* for platform abstraction)
    void* getNativeHandle() const { return m_hwnd; }
    void* getNativeDeviceContext() const { return m_hdc; }
    void* getNativeGLContext() const { return m_hglrc; }

private:
    // Window procedure (implemented in .cpp where Windows types are available)
    static long long __stdcall WindowProc(void* hwnd, unsigned int msg, unsigned long long wParam, long long lParam);
    long long handleMessage(unsigned int msg, unsigned long long wParam, long long lParam);
    
    // Helper methods
    bool registerWindowClass();
    void setupPixelFormat();
    void handleMouseMove(int x, int y);
    void handleMouseButton(int button, bool pressed);
    void handleMouseWheel(float delta);
    void handleKey(int key, bool pressed);
    void handleResize(int width, int height);
    
    // Window handles (void* for platform abstraction)
    void* m_hwnd;
    void* m_hdc;
    void* m_hglrc;
    
    // Window properties
    std::string m_title;
    int m_width;
    int m_height;
    bool m_shouldClose;
    
    // Components
    NUIComponent* m_rootComponent;
    NUIRenderer* m_renderer;
    
    // Event callbacks
    std::function<void(int, int)> m_mouseMoveCallback;
    std::function<void(int, bool)> m_mouseButtonCallback;
    std::function<void(float)> m_mouseWheelCallback;
    std::function<void(int, bool)> m_keyCallback;
    std::function<void(int, int)> m_resizeCallback;
    std::function<void()> m_closeCallback;
    
    // Mouse state
    int m_mouseX;
    int m_mouseY;
    
    // Static window class name
    static const wchar_t* WINDOW_CLASS_NAME;
    static bool s_classRegistered;
};

} // namespace NomadUI
