#include "../../External/glad/include/glad/glad.h"  // MUST be included before Windows.h

// Prevent Windows from defining conflicting macros
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#define NOCOMM

// Suppress APIENTRY redefinition warning
#pragma warning(push)
#pragma warning(disable: 4005)

#include "NUIWindowWin32.h"
#include "../../Graphics/NUIRenderer.h"
#include "../../Core/NUIComponent.h"
#include <Windows.h>
#include <windowsx.h>  // For GET_X_LPARAM, GET_Y_LPARAM
#include <dwmapi.h>    // For DwmExtendFrameIntoClientArea
#include <iostream>

#pragma comment(lib, "dwmapi.lib")  // Link DWM API library

// Restore warnings
#pragma warning(pop)

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
    , m_isFullScreen(false)
    , m_restoreX(0), m_restoreY(0), m_restoreWidth(0), m_restoreHeight(0)
    , m_restoreStyle(0)
    , m_snapState(SnapState::None)
    , m_isDragging(false)
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

bool NUIWindowWin32::create(const std::string& title, int width, int height, bool startMaximized) {
    // Set DPI awareness to avoid scaling issues
    SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
    
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
    
    // Calculate center position (no borders for borderless window)
    int screenWidth = GetSystemMetrics(SM_CXSCREEN);
    int screenHeight = GetSystemMetrics(SM_CYSCREEN);
    int x = (screenWidth - width) / 2;
    int y = (screenHeight - height) / 2;

    // Create truly borderless window with maximize support
    // Use WS_POPUP without WS_THICKFRAME to eliminate invisible borders
    // We handle resizing manually via WM_NCHITTEST
    m_hwnd = CreateWindowExW(
        WS_EX_APPWINDOW,  // Ensure it appears in taskbar
        WINDOW_CLASS_NAME,
        wideTitle,
        WS_POPUP | WS_MAXIMIZEBOX | WS_MINIMIZEBOX | WS_SYSMENU,  // No WS_THICKFRAME = no invisible borders
        x, y,
        width,
        height,
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
    
    // Use DWM to extend frame into client area for borderless appearance
    // Set margins to 0 to make the entire window the client area
    MARGINS margins = {0, 0, 0, 0};
    DwmExtendFrameIntoClientArea(TO_HWND(m_hwnd), &margins);
    
    // Disable DWM window shadows to prevent any visual artifacts
    DWMNCRENDERINGPOLICY policy = DWMNCRP_DISABLED;
    DwmSetWindowAttribute(TO_HWND(m_hwnd), DWMWA_NCRENDERING_POLICY, &policy, sizeof(policy));
    
    // Ensure DWM composition is enabled for proper rendering
    BOOL composition = TRUE;
    DwmSetWindowAttribute(TO_HWND(m_hwnd), DWMWA_NCRENDERING_ENABLED, &composition, sizeof(composition));
    
    // Force window size to exactly what we want
    // Our WM_NCCALCSIZE handler makes client area = window area, so just set window size
    SetWindowPos(TO_HWND(m_hwnd), nullptr, x, y, width, height, 
                 SWP_NOZORDER | SWP_NOACTIVATE | SWP_FRAMECHANGED);
    
    // Ensure our internal size tracking is correct
    m_width = width;
    m_height = height;
    
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
    
    // Start maximized if requested (optional)
    if (startMaximized) {
        // Note: User can manually maximize or use window.maximize() after creation
        ShowWindow(TO_HWND(m_hwnd), SW_SHOWMAXIMIZED);
        std::cout << "Window started maximized" << std::endl;
    }
    
    return true;
}

void NUIWindowWin32::setupPixelFormat() {
    // Try to use MSAA for better anti-aliasing
    // Note: This requires WGL extensions which are only available after creating a context
    // For now, we'll use the basic pixel format and enable MSAA in the renderer if supported
    
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
    
    // TODO: For Phase 2 MSAA implementation:
    // 1. Create temporary context
    // 2. Load wglChoosePixelFormatARB
    // 3. Destroy temporary context and window
    // 4. Create new window with MSAA pixel format
    // 5. Create final context with MSAA enabled
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
        // No need to adjust for borders since we're using a borderless window
        SetWindowPos(TO_HWND(m_hwnd), nullptr, 0, 0, width, height,
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

void NUIWindowWin32::getPosition(int& x, int& y) const {
    if (m_hwnd) {
        RECT rect;
        GetWindowRect(TO_HWND(m_hwnd), &rect);
        x = rect.left;
        y = rect.top;
    } else {
        x = 0;
        y = 0;
    }
}

void NUIWindowWin32::minimize() {
    if (m_hwnd) {
        // If maximized, restore first then minimize
        if (isMaximized()) {
            ShowWindow(TO_HWND(m_hwnd), SW_RESTORE);
            // Small delay to ensure restore completes
            Sleep(10);
        }
        ShowWindow(TO_HWND(m_hwnd), SW_MINIMIZE);
    }
}

void NUIWindowWin32::maximize() {
    if (m_hwnd) {
        if (isMaximized()) {
            ShowWindow(TO_HWND(m_hwnd), SW_RESTORE);
        } else {
            ShowWindow(TO_HWND(m_hwnd), SW_MAXIMIZE);
        }
    }
}

void NUIWindowWin32::restore() {
    if (m_hwnd) {
        ShowWindow(TO_HWND(m_hwnd), SW_RESTORE);
    }
}

bool NUIWindowWin32::isMaximized() const {
    if (m_hwnd) {
        WINDOWPLACEMENT placement = {};
        placement.length = sizeof(WINDOWPLACEMENT);
        GetWindowPlacement(TO_HWND(m_hwnd), &placement);
        return placement.showCmd == SW_SHOWMAXIMIZED;
    }
    return false;
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
        
        case WM_NCCALCSIZE: {
            // Handle non-client area for truly borderless window
            if (wParam == TRUE) {
                NCCALCSIZE_PARAMS* params = reinterpret_cast<NCCALCSIZE_PARAMS*>(lParam);
                
                // When maximized, adjust for the taskbar
                if (isMaximized()) {
                    // Get the monitor info to account for taskbar
                    HMONITOR monitor = MonitorFromWindow(TO_HWND(m_hwnd), MONITOR_DEFAULTTONEAREST);
                    MONITORINFO monitorInfo = {};
                    monitorInfo.cbSize = sizeof(MONITORINFO);
                    GetMonitorInfo(monitor, &monitorInfo);
                    
                    // Set client area to work area (screen minus taskbar)
                    params->rgrc[0] = monitorInfo.rcWork;
                } else {
                    // For normal windowed mode, the client area equals the window area
                    // params->rgrc[0] already contains the new window rect
                    // params->rgrc[1] contains the old window rect
                    // params->rgrc[2] contains the old client rect
                    // By not modifying rgrc[0], we're saying: client area = window area
                    // This eliminates the invisible borders that WS_THICKFRAME adds
                }
                // Return 0 to indicate we've handled the calculation
                return 0;
            }
            break;
        }
        
        case WM_NCHITTEST: {
            // Custom hit testing for borderless window with resize support
            POINT pt = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
            ScreenToClient(static_cast<HWND>(m_hwnd), &pt);
            
            // Don't allow resizing when maximized
            if (!isMaximized()) {
                // Define resize border width
                const int resizeBorderWidth = 8;
                
                // Check for corner and edge resize areas
                bool isLeft = pt.x < resizeBorderWidth;
                bool isRight = pt.x >= m_width - resizeBorderWidth;
                bool isTop = pt.y < resizeBorderWidth;
                bool isBottom = pt.y >= m_height - resizeBorderWidth;
                
                // Return appropriate resize cursor positions
                if (isTop && isLeft) return HTTOPLEFT;
                if (isTop && isRight) return HTTOPRIGHT;
                if (isBottom && isLeft) return HTBOTTOMLEFT;
                if (isBottom && isRight) return HTBOTTOMRIGHT;
                if (isTop) return HTTOP;
                if (isBottom) return HTBOTTOM;
                if (isLeft) return HTLEFT;
                if (isRight) return HTRIGHT;
            }
            
            // Check if in title bar area (top 32 pixels)
            // But exclude the right 150 pixels for window controls (3 buttons * 46px + margin)
            if (pt.y >= 0 && pt.y < 32 && pt.x < m_width - 150) {
                return HTCAPTION; // Treat as title bar for dragging
            }
            
            return HTCLIENT;
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
        
        case WM_ENTERSIZEMOVE:
            // Window drag/resize started
            m_isDragging = true;
            return 0;
        
        case WM_EXITSIZEMOVE: {
            // Window drag/resize ended - check for snap
            m_isDragging = false;
            
            // Get cursor position
            POINT cursorPos;
            GetCursorPos(&cursorPos);
            
            // Check if we should snap
            checkSnapZones(cursorPos.x, cursorPos.y);
            return 0;
        }
        
        case WM_MOVING: {
            // Window is being moved - could show snap preview here
            // For now, just track that we're dragging
            m_isDragging = true;
            return 0;
        }
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

NUIKeyCode NUIWindowWin32::convertKeyCode(int windowsKey) {
    switch (windowsKey) {
        case VK_SPACE: return NUIKeyCode::Space;
        case VK_RETURN: return NUIKeyCode::Enter;
        case VK_ESCAPE: return NUIKeyCode::Escape;
        case VK_TAB: return NUIKeyCode::Tab;
        case VK_BACK: return NUIKeyCode::Backspace;
        case VK_DELETE: return NUIKeyCode::Delete;
        case VK_LEFT: return NUIKeyCode::Left;
        case VK_RIGHT: return NUIKeyCode::Right;
        case VK_UP: return NUIKeyCode::Up;
        case VK_DOWN: return NUIKeyCode::Down;
        case 'A': return NUIKeyCode::A;
        case 'B': return NUIKeyCode::B;
        case 'C': return NUIKeyCode::C;
        case 'D': return NUIKeyCode::D;
        case 'E': return NUIKeyCode::E;
        case 'F': return NUIKeyCode::F;
        case 'G': return NUIKeyCode::G;
        case 'H': return NUIKeyCode::H;
        case 'I': return NUIKeyCode::I;
        case 'J': return NUIKeyCode::J;
        case 'K': return NUIKeyCode::K;
        case 'L': return NUIKeyCode::L;
        case 'M': return NUIKeyCode::M;
        case 'N': return NUIKeyCode::N;
        case 'O': return NUIKeyCode::O;
        case 'P': return NUIKeyCode::P;
        case 'Q': return NUIKeyCode::Q;
        case 'R': return NUIKeyCode::R;
        case 'S': return NUIKeyCode::S;
        case 'T': return NUIKeyCode::T;
        case 'U': return NUIKeyCode::U;
        case 'V': return NUIKeyCode::V;
        case 'W': return NUIKeyCode::W;
        case 'X': return NUIKeyCode::X;
        case 'Y': return NUIKeyCode::Y;
        case 'Z': return NUIKeyCode::Z;
        case '0': return NUIKeyCode::Num0;
        case '1': return NUIKeyCode::Num1;
        case '2': return NUIKeyCode::Num2;
        case '3': return NUIKeyCode::Num3;
        case '4': return NUIKeyCode::Num4;
        case '5': return NUIKeyCode::Num5;
        case '6': return NUIKeyCode::Num6;
        case '7': return NUIKeyCode::Num7;
        case '8': return NUIKeyCode::Num8;
        case '9': return NUIKeyCode::Num9;
        case VK_F1: return NUIKeyCode::F1;
        case VK_F2: return NUIKeyCode::F2;
        case VK_F3: return NUIKeyCode::F3;
        case VK_F4: return NUIKeyCode::F4;
        case VK_F5: return NUIKeyCode::F5;
        case VK_F6: return NUIKeyCode::F6;
        case VK_F7: return NUIKeyCode::F7;
        case VK_F8: return NUIKeyCode::F8;
        case VK_F9: return NUIKeyCode::F9;
        case VK_F10: return NUIKeyCode::F10;
        case VK_F11: return NUIKeyCode::F11;
        case VK_F12: return NUIKeyCode::F12;
        default: return NUIKeyCode::Unknown;
    }
}

void NUIWindowWin32::handleKey(int key, bool pressed) {
    if (m_keyCallback) {
        m_keyCallback(key, pressed);
    }
    
    // Forward to root component
    if (m_rootComponent) {
        NUIKeyEvent event;
        event.keyCode = convertKeyCode(key);
        event.pressed = pressed;
        event.released = !pressed;
        m_rootComponent->onKeyEvent(event);
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
    
    // Update renderer projection matrix and viewport
    if (m_renderer) {
        m_renderer->resize(width, height);
    }
    
    // Update OpenGL viewport
    if (m_hglrc) {
        makeContextCurrent();
        if (glViewport) {  // Check if OpenGL is loaded
            glViewport(0, 0, width, height);
        }
    }
}

void NUIWindowWin32::toggleFullScreen() {
    if (m_isFullScreen) {
        exitFullScreen();
    } else {
        enterFullScreen();
    }
}

void NUIWindowWin32::enterFullScreen() {
    if (m_isFullScreen || !m_hwnd) return;
    
    // Store current window state
    RECT rect;
    GetWindowRect(TO_HWND(m_hwnd), &rect);
    m_restoreX = rect.left;
    m_restoreY = rect.top;
    m_restoreWidth = rect.right - rect.left;
    m_restoreHeight = rect.bottom - rect.top;
    m_restoreStyle = GetWindowLong(TO_HWND(m_hwnd), GWL_STYLE);
    
    // Get screen dimensions
    int screenWidth = GetSystemMetrics(SM_CXSCREEN);
    int screenHeight = GetSystemMetrics(SM_CYSCREEN);
    
    // Remove title bar and borders
    DWORD newStyle = WS_POPUP | WS_VISIBLE;
    SetWindowLong(TO_HWND(m_hwnd), GWL_STYLE, newStyle);
    
    // Set window to cover entire screen
    SetWindowPos(TO_HWND(m_hwnd), HWND_TOP, 0, 0, screenWidth, screenHeight, 
                 SWP_FRAMECHANGED | SWP_SHOWWINDOW);
    
    m_isFullScreen = true;
    m_width = screenWidth;
    m_height = screenHeight;
    
    // Update root component bounds
    if (m_rootComponent) {
        m_rootComponent->setBounds(NUIRect(0, 0, screenWidth, screenHeight));
    }
    
    // Update OpenGL viewport and projection matrix
    if (m_hglrc) {
        makeContextCurrent();
        if (glViewport) {
            glViewport(0, 0, screenWidth, screenHeight);
        }
        // Update renderer projection matrix for new dimensions
        if (m_renderer) {
            m_renderer->resize(screenWidth, screenHeight);
        }
    }
}

void NUIWindowWin32::exitFullScreen() {
    if (!m_isFullScreen || !m_hwnd) return;
    
    // Restore original window style
    SetWindowLong(TO_HWND(m_hwnd), GWL_STYLE, m_restoreStyle);
    
    // Restore original position and size
    SetWindowPos(TO_HWND(m_hwnd), HWND_NOTOPMOST, m_restoreX, m_restoreY, 
                 m_restoreWidth, m_restoreHeight, 
                 SWP_FRAMECHANGED | SWP_SHOWWINDOW);
    
    m_isFullScreen = false;
    m_width = m_restoreWidth;
    m_height = m_restoreHeight;
    
    // Update root component bounds
    if (m_rootComponent) {
        m_rootComponent->setBounds(NUIRect(0, 0, m_restoreWidth, m_restoreHeight));
    }
    
    // Update OpenGL viewport and projection matrix
    if (m_hglrc) {
        makeContextCurrent();
        if (glViewport) {
            glViewport(0, 0, m_restoreWidth, m_restoreHeight);
        }
        // Update renderer projection matrix for new dimensions
        if (m_renderer) {
            m_renderer->resize(m_restoreWidth, m_restoreHeight);
        }
    }
}

// Windows Snap functionality
void NUIWindowWin32::checkSnapZones(int x, int y) {
    if (!m_hwnd || m_isFullScreen) return;
    
    // Get screen dimensions
    int screenWidth = GetSystemMetrics(SM_CXSCREEN);
    int screenHeight = GetSystemMetrics(SM_CYSCREEN);
    
    // Define snap threshold (pixels from edge)
    const int snapThreshold = 20;
    
    // Check which snap zone the cursor is in
    bool atTop = y < snapThreshold;
    bool atBottom = y > screenHeight - snapThreshold;
    bool atLeft = x < snapThreshold;
    bool atRight = x > screenWidth - snapThreshold;
    
    SnapState newSnap = SnapState::None;
    
    // Corner snaps (quarter screen)
    if (atTop && atLeft) {
        newSnap = SnapState::TopLeft;
    } else if (atTop && atRight) {
        newSnap = SnapState::TopRight;
    } else if (atBottom && atLeft) {
        newSnap = SnapState::BottomLeft;
    } else if (atBottom && atRight) {
        newSnap = SnapState::BottomRight;
    }
    // Edge snaps (half screen)
    else if (atTop) {
        // Top edge maximizes
        if (m_snapState != SnapState::None) {
            restoreFromSnap();
        }
        maximize();
        return;
    } else if (atLeft) {
        newSnap = SnapState::Left;
    } else if (atRight) {
        newSnap = SnapState::Right;
    }
    
    // Apply snap if changed
    if (newSnap != m_snapState && newSnap != SnapState::None) {
        applySnap(newSnap);
    }
}

void NUIWindowWin32::applySnap(SnapState snap) {
    if (!m_hwnd || m_isFullScreen) return;
    
    // Store current position before snapping (if not already snapped)
    if (m_snapState == SnapState::None && !isMaximized()) {
        RECT rect;
        GetWindowRect(TO_HWND(m_hwnd), &rect);
        m_restoreX = rect.left;
        m_restoreY = rect.top;
        m_restoreWidth = rect.right - rect.left;
        m_restoreHeight = rect.bottom - rect.top;
    }
    
    // Get work area (screen minus taskbar)
    RECT workArea;
    SystemParametersInfo(SPI_GETWORKAREA, 0, &workArea, 0);
    
    int screenX = workArea.left;
    int screenY = workArea.top;
    int screenWidth = workArea.right - workArea.left;
    int screenHeight = workArea.bottom - workArea.top;
    
    int newX, newY, newWidth, newHeight;
    
    switch (snap) {
        case SnapState::Left:
            newX = screenX;
            newY = screenY;
            newWidth = screenWidth / 2;
            newHeight = screenHeight;
            break;
            
        case SnapState::Right:
            newX = screenX + screenWidth / 2;
            newY = screenY;
            newWidth = screenWidth / 2;
            newHeight = screenHeight;
            break;
            
        case SnapState::TopLeft:
            newX = screenX;
            newY = screenY;
            newWidth = screenWidth / 2;
            newHeight = screenHeight / 2;
            break;
            
        case SnapState::TopRight:
            newX = screenX + screenWidth / 2;
            newY = screenY;
            newWidth = screenWidth / 2;
            newHeight = screenHeight / 2;
            break;
            
        case SnapState::BottomLeft:
            newX = screenX;
            newY = screenY + screenHeight / 2;
            newWidth = screenWidth / 2;
            newHeight = screenHeight / 2;
            break;
            
        case SnapState::BottomRight:
            newX = screenX + screenWidth / 2;
            newY = screenY + screenHeight / 2;
            newWidth = screenWidth / 2;
            newHeight = screenHeight / 2;
            break;
            
        default:
            return;
    }
    
    // Apply the snap
    SetWindowPos(TO_HWND(m_hwnd), nullptr, newX, newY, newWidth, newHeight,
                 SWP_NOZORDER | SWP_NOACTIVATE | SWP_FRAMECHANGED);
    
    m_snapState = snap;
    m_width = newWidth;
    m_height = newHeight;
    
    // Update root component
    if (m_rootComponent) {
        m_rootComponent->setBounds(NUIRect(0, 0, newWidth, newHeight));
    }
    
    // Update renderer
    if (m_renderer) {
        m_renderer->resize(newWidth, newHeight);
    }
    
    std::cout << "Window snapped to ";
    switch (snap) {
        case SnapState::Left: std::cout << "left half"; break;
        case SnapState::Right: std::cout << "right half"; break;
        case SnapState::TopLeft: std::cout << "top-left quarter"; break;
        case SnapState::TopRight: std::cout << "top-right quarter"; break;
        case SnapState::BottomLeft: std::cout << "bottom-left quarter"; break;
        case SnapState::BottomRight: std::cout << "bottom-right quarter"; break;
        default: break;
    }
    std::cout << std::endl;
}

void NUIWindowWin32::restoreFromSnap() {
    if (!m_hwnd || m_snapState == SnapState::None) return;
    
    // Restore to previous position and size
    SetWindowPos(TO_HWND(m_hwnd), nullptr, m_restoreX, m_restoreY, 
                 m_restoreWidth, m_restoreHeight,
                 SWP_NOZORDER | SWP_NOACTIVATE | SWP_FRAMECHANGED);
    
    m_snapState = SnapState::None;
    m_width = m_restoreWidth;
    m_height = m_restoreHeight;
    
    // Update root component
    if (m_rootComponent) {
        m_rootComponent->setBounds(NUIRect(0, 0, m_restoreWidth, m_restoreHeight));
    }
    
    // Update renderer
    if (m_renderer) {
        m_renderer->resize(m_restoreWidth, m_restoreHeight);
    }
    
    std::cout << "Window restored from snap" << std::endl;
}


} // namespace NomadUI
