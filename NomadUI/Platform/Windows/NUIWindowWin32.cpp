#include <glad/glad.h>  // MUST be included before Windows.h

// Prevent Windows from defining conflicting macros
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#define NOCOMM

#include "NUIWindowWin32.h"
#include "../../Graphics/NUIRenderer.h"
#include "../../Core/NUIComponent.h"
#include <Windows.h>
#include <windowsx.h>  // For GET_X_LPARAM, GET_Y_LPARAM
#include <iostream>

namespace NomadUI {

// Helper macros to cast void* to Windows types
#define TO_HWND(ptr) static_cast<HWND>(ptr)
#define TO_HDC(ptr) static_cast<HDC>(ptr)
#define TO_HGLRC(ptr) static_cast<HGLRC>(ptr)

// Static members
const wchar_t* NUIWindowWin32::WINDOW_CLASS_NAME = L"NomadUIWindow";
bool NUIWindowWin32::s_classRegistered = false;

NUIWindowWin32::NUIWindowWin32()
    : m_hwnd(nullptr)
    , m_hdc(nullptr)
    , m_hglrc(nullptr)
    , m_width(800)
    , m_height(600)
    , m_shouldClose(false)
    , m_rootComponent(nullptr)
    , m_renderer(nullptr)
    , m_mouseX(0)
    , m_mouseY(0)
{
}

NUIWindowWin32::~NUIWindowWin32() {
    destroy();
}

bool NUIWindowWin32::registerWindowClass() {
    if (s_classRegistered) {
        return true;
    }
    
    WNDCLASSEXW wc = {};
    wc.cbSize = sizeof(WNDCLASSEXW);
    wc.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
    wc.lpfnWndProc = reinterpret_cast<WNDPROC>(WindowProc);
    wc.hInstance = GetModuleHandle(nullptr);
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.lpszClassName = WINDOW_CLASS_NAME;
    
    if (!RegisterClassExW(&wc)) {
        std::cerr << "Failed to register window class" << std::endl;
        return false;
    }
    
    s_classRegistered = true;
    return true;
}

bool NUIWindowWin32::create(const std::string& title, int width, int height) {
    m_title = title;
    m_width = width;
    m_height = height;
    
    // Register window class
    if (!registerWindowClass()) {
        return false;
    }
    
    // Convert title to wide string
    int titleLen = MultiByteToWideChar(CP_UTF8, 0, title.c_str(), -1, nullptr, 0);
    wchar_t* wideTitle = new wchar_t[titleLen];
    MultiByteToWideChar(CP_UTF8, 0, title.c_str(), -1, wideTitle, titleLen);
    
    // Calculate window size including borders
    RECT rect = { 0, 0, width, height };
    AdjustWindowRect(&rect, WS_OVERLAPPEDWINDOW, FALSE);
    
    // Calculate center position
    int screenWidth = GetSystemMetrics(SM_CXSCREEN);
    int screenHeight = GetSystemMetrics(SM_CYSCREEN);
    int windowWidth = rect.right - rect.left;
    int windowHeight = rect.bottom - rect.top;
    int x = (screenWidth - windowWidth) / 2;
    int y = (screenHeight - windowHeight) / 2;

    // Create window
    m_hwnd = CreateWindowExW(
        0,
        WINDOW_CLASS_NAME,
        wideTitle,
        WS_OVERLAPPEDWINDOW,
        x, y,  // Center the window
        windowWidth,
        windowHeight,
        nullptr,
        nullptr,
        GetModuleHandle(nullptr),
        this  // Pass 'this' pointer to WM_CREATE
    );
    
    delete[] wideTitle;
    
    if (!m_hwnd) {
        std::cerr << "Failed to create window" << std::endl;
        return false;
    }
    
    // Get device context
    m_hdc = GetDC(TO_HWND(m_hwnd));
    if (!m_hdc) {
        std::cerr << "Failed to get device context" << std::endl;
        destroy();
        return false;
    }
    
    // Setup pixel format for OpenGL
    setupPixelFormat();
    
    // Create OpenGL context
    if (!createGLContext()) {
        destroy();
        return false;
    }
    
    return true;
}

void NUIWindowWin32::setupPixelFormat() {
    PIXELFORMATDESCRIPTOR pfd = {};
    pfd.nSize = sizeof(PIXELFORMATDESCRIPTOR);
    pfd.nVersion = 1;
    pfd.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
    pfd.iPixelType = PFD_TYPE_RGBA;
    pfd.cColorBits = 32;
    pfd.cDepthBits = 24;
    pfd.cStencilBits = 8;
    pfd.iLayerType = PFD_MAIN_PLANE;
    
    int pixelFormat = ChoosePixelFormat(TO_HDC(m_hdc), &pfd);
    if (!pixelFormat) {
        std::cerr << "Failed to choose pixel format" << std::endl;
        return;
    }
    
    if (!SetPixelFormat(TO_HDC(m_hdc), pixelFormat, &pfd)) {
        std::cerr << "Failed to set pixel format" << std::endl;
    }
}

bool NUIWindowWin32::createGLContext() {
    // Create temporary context for loading extensions
    HGLRC tempContext = wglCreateContext(TO_HDC(m_hdc));
    if (!tempContext) {
        std::cerr << "Failed to create temporary OpenGL context" << std::endl;
        return false;
    }
    
    wglMakeCurrent(TO_HDC(m_hdc), tempContext);
    
    // Load OpenGL functions with GLAD
    if (!gladLoadGL()) {
        std::cerr << "Failed to initialize GLAD" << std::endl;
        wglMakeCurrent(nullptr, nullptr);
        wglDeleteContext(tempContext);
        return false;
    }
    
    // Try to create modern OpenGL context (3.3+)
    // For now, we'll use the basic context
    // TODO: Use wglCreateContextAttribsARB for 3.3+ context
    m_hglrc = tempContext;
    
    std::cout << "OpenGL context created successfully!" << std::endl;
    
    return true;
}

bool NUIWindowWin32::makeContextCurrent() {
    return wglMakeCurrent(TO_HDC(m_hdc), TO_HGLRC(m_hglrc)) == TRUE;
}

void NUIWindowWin32::destroy() {
    if (m_hglrc) {
        wglMakeCurrent(nullptr, nullptr);
        wglDeleteContext(TO_HGLRC(m_hglrc));
        m_hglrc = nullptr;
    }
    
    if (m_hdc) {
        ReleaseDC(TO_HWND(m_hwnd), TO_HDC(m_hdc));
        m_hdc = nullptr;
    }
    
    if (m_hwnd) {
        DestroyWindow(TO_HWND(m_hwnd));
        m_hwnd = nullptr;
    }
}

void NUIWindowWin32::show() {
    if (m_hwnd) {
        ShowWindow(TO_HWND(m_hwnd), SW_SHOW);
        UpdateWindow(TO_HWND(m_hwnd));
    }
}

void NUIWindowWin32::hide() {
    if (m_hwnd) {
        ShowWindow(TO_HWND(m_hwnd), SW_HIDE);
    }
}

bool NUIWindowWin32::processEvents() {
    MSG msg;
    while (PeekMessageW(&msg, nullptr, 0, 0, PM_REMOVE)) {
        if (msg.message == WM_QUIT) {
            m_shouldClose = true;
            return false;
        }
        
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }
    
    return !m_shouldClose;
}

void NUIWindowWin32::swapBuffers() {
    if (m_hdc) {
        SwapBuffers(TO_HDC(m_hdc));
    }
}

void NUIWindowWin32::setTitle(const std::string& title) {
    m_title = title;
    if (m_hwnd) {
        int titleLen = MultiByteToWideChar(CP_UTF8, 0, title.c_str(), -1, nullptr, 0);
        wchar_t* wideTitle = new wchar_t[titleLen];
        MultiByteToWideChar(CP_UTF8, 0, title.c_str(), -1, wideTitle, titleLen);
        SetWindowTextW(TO_HWND(m_hwnd), wideTitle);
        delete[] wideTitle;
    }
}

void NUIWindowWin32::setSize(int width, int height) {
    m_width = width;
    m_height = height;
    if (m_hwnd) {
        RECT rect = { 0, 0, width, height };
        AdjustWindowRect(&rect, WS_OVERLAPPEDWINDOW, FALSE);
        SetWindowPos(TO_HWND(m_hwnd), nullptr, 0, 0, 
                    rect.right - rect.left, rect.bottom - rect.top,
                    SWP_NOMOVE | SWP_NOZORDER);
    }
}

void NUIWindowWin32::getSize(int& width, int& height) const {
    width = m_width;
    height = m_height;
}

void NUIWindowWin32::setPosition(int x, int y) {
    if (m_hwnd) {
        SetWindowPos(TO_HWND(m_hwnd), nullptr, x, y, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
    }
}

void NUIWindowWin32::setRootComponent(NUIComponent* root) {
    m_rootComponent = root;
    if (m_rootComponent) {
        m_rootComponent->setBounds(NUIRect(0, 0, m_width, m_height));
    }
}

void NUIWindowWin32::setRenderer(NUIRenderer* renderer) {
    m_renderer = renderer;
}

// Event callback setters
void NUIWindowWin32::setMouseMoveCallback(std::function<void(int, int)> callback) {
    m_mouseMoveCallback = callback;
}

void NUIWindowWin32::setMouseButtonCallback(std::function<void(int, bool)> callback) {
    m_mouseButtonCallback = callback;
}

void NUIWindowWin32::setMouseWheelCallback(std::function<void(float)> callback) {
    m_mouseWheelCallback = callback;
}

void NUIWindowWin32::setKeyCallback(std::function<void(int, bool)> callback) {
    m_keyCallback = callback;
}

void NUIWindowWin32::setResizeCallback(std::function<void(int, int)> callback) {
    m_resizeCallback = callback;
}

void NUIWindowWin32::setCloseCallback(std::function<void()> callback) {
    m_closeCallback = callback;
}

// Window procedure (matching the header signature)
long long __stdcall NUIWindowWin32::WindowProc(void* hwnd, unsigned int msg, unsigned long long wParam, long long lParam) {
    HWND hwndWin = static_cast<HWND>(hwnd);
    NUIWindowWin32* window = nullptr;
    
    if (msg == WM_CREATE) {
        CREATESTRUCT* cs = reinterpret_cast<CREATESTRUCT*>(lParam);
        window = reinterpret_cast<NUIWindowWin32*>(cs->lpCreateParams);
        SetWindowLongPtr(hwndWin, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(window));
    } else {
        window = reinterpret_cast<NUIWindowWin32*>(GetWindowLongPtr(hwndWin, GWLP_USERDATA));
    }
    
    if (window) {
        return window->handleMessage(msg, wParam, lParam);
    }
    
    return DefWindowProcW(hwndWin, static_cast<UINT>(msg), static_cast<WPARAM>(wParam), static_cast<LPARAM>(lParam));
}

long long NUIWindowWin32::handleMessage(unsigned int msg, unsigned long long wParam, long long lParam) {
    switch (msg) {
        case WM_CLOSE:
            m_shouldClose = true;
            if (m_closeCallback) {
                m_closeCallback();
            }
            return 0;
            
        case WM_SIZE: {
            int width = LOWORD(lParam);
            int height = HIWORD(lParam);
            handleResize(width, height);
            return 0;
        }
        
        case WM_MOUSEMOVE: {
            int x = GET_X_LPARAM(lParam);
            int y = GET_Y_LPARAM(lParam);
            handleMouseMove(x, y);
            return 0;
        }
        
        case WM_LBUTTONDOWN:
            handleMouseButton(0, true);
            return 0;
            
        case WM_LBUTTONUP:
            handleMouseButton(0, false);
            return 0;
            
        case WM_RBUTTONDOWN:
            handleMouseButton(1, true);
            return 0;
            
        case WM_RBUTTONUP:
            handleMouseButton(1, false);
            return 0;
            
        case WM_MBUTTONDOWN:
            handleMouseButton(2, true);
            return 0;
            
        case WM_MBUTTONUP:
            handleMouseButton(2, false);
            return 0;
            
        case WM_MOUSEWHEEL: {
            float delta = GET_WHEEL_DELTA_WPARAM(wParam) / 120.0f;
            handleMouseWheel(delta);
            return 0;
        }
        
        case WM_KEYDOWN:
            handleKey(static_cast<int>(wParam), true);
            return 0;
            
        case WM_KEYUP:
            handleKey(static_cast<int>(wParam), false);
            return 0;
    }
    
    return DefWindowProcW(static_cast<HWND>(m_hwnd), static_cast<UINT>(msg), static_cast<WPARAM>(wParam), static_cast<LPARAM>(lParam));
}

void NUIWindowWin32::handleMouseMove(int x, int y) {
    m_mouseX = x;
    m_mouseY = y;
    
    if (m_mouseMoveCallback) {
        m_mouseMoveCallback(x, y);
    }
    
    // Forward to root component
    if (m_rootComponent) {
        NUIMouseEvent event;
        event.position = {static_cast<float>(x), static_cast<float>(y)};
        event.button = NUIMouseButton::None;
        event.pressed = false;
        event.released = false;
        m_rootComponent->onMouseEvent(event);
    }
}

void NUIWindowWin32::handleMouseButton(int button, bool pressed) {
    if (m_mouseButtonCallback) {
        m_mouseButtonCallback(button, pressed);
    }
    
    // Forward to root component
    if (m_rootComponent) {
        NUIMouseEvent event;
        event.position = {static_cast<float>(m_mouseX), static_cast<float>(m_mouseY)};
        // Map Windows button values to NUIMouseButton
        switch (button) {
            case 0: event.button = NUIMouseButton::Left; break;
            case 1: event.button = NUIMouseButton::Right; break;
            case 2: event.button = NUIMouseButton::Middle; break;
            default: event.button = NUIMouseButton::None; break;
        }
        event.pressed = pressed;
        event.released = !pressed;
        m_rootComponent->onMouseEvent(event);
    }
}

void NUIWindowWin32::handleMouseWheel(float delta) {
    if (m_mouseWheelCallback) {
        m_mouseWheelCallback(delta);
    }
}

void NUIWindowWin32::handleKey(int key, bool pressed) {
    if (m_keyCallback) {
        m_keyCallback(key, pressed);
    }
}

void NUIWindowWin32::handleResize(int width, int height) {
    m_width = width;
    m_height = height;
    
    if (m_resizeCallback) {
        m_resizeCallback(width, height);
    }
    
    if (m_rootComponent) {
        m_rootComponent->setBounds(NUIRect(0, 0, width, height));
    }
    
    // Update OpenGL viewport
    if (m_hglrc) {
        makeContextCurrent();
        if (glViewport) {  // Check if OpenGL is loaded
            glViewport(0, 0, width, height);
        }
    }
}

} // namespace NomadUI
