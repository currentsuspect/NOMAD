/**
 * @file window.hpp
 * @brief Window abstraction interface for Nomad DAW
 * @author Nomad Team
 * @date 2025
 * 
 * This file defines the window and input abstraction interfaces.
 * Platform-specific implementations use native APIs:
 * - Windows: Win32
 * - macOS: Cocoa
 * - Linux: X11 or Wayland
 */

#pragma once

#include "../../core/base/types.hpp"
#include "../../core/base/config.hpp"

#include <string>
#include <string_view>
#include <memory>
#include <functional>

namespace nomad::platform {

//=============================================================================
// Input Types
//=============================================================================

/**
 * @brief Mouse button identifiers
 */
enum class MouseButton : u8 {
    None,
    Left,
    Right,
    Middle,
    Back,
    Forward
};

/**
 * @brief Keyboard modifier flags
 */
enum class ModifierFlags : u8 {
    None    = 0,
    Shift   = 1 << 0,
    Control = 1 << 1,
    Alt     = 1 << 2,
    Super   = 1 << 3  // Windows key / Command key
};

inline ModifierFlags operator|(ModifierFlags a, ModifierFlags b) {
    return static_cast<ModifierFlags>(static_cast<u8>(a) | static_cast<u8>(b));
}

inline ModifierFlags operator&(ModifierFlags a, ModifierFlags b) {
    return static_cast<ModifierFlags>(static_cast<u8>(a) & static_cast<u8>(b));
}

inline bool hasModifier(ModifierFlags flags, ModifierFlags test) {
    return (static_cast<u8>(flags) & static_cast<u8>(test)) != 0;
}

/**
 * @brief Key codes (platform-independent)
 */
enum class KeyCode : u16 {
    Unknown = 0,
    
    // Letters
    A, B, C, D, E, F, G, H, I, J, K, L, M,
    N, O, P, Q, R, S, T, U, V, W, X, Y, Z,
    
    // Numbers
    Num0, Num1, Num2, Num3, Num4, Num5, Num6, Num7, Num8, Num9,
    
    // Function keys
    F1, F2, F3, F4, F5, F6, F7, F8, F9, F10, F11, F12,
    
    // Navigation
    Escape, Tab, CapsLock, Backspace, Enter, Space,
    Insert, Delete, Home, End, PageUp, PageDown,
    Left, Right, Up, Down,
    
    // Modifiers
    LeftShift, RightShift, LeftControl, RightControl,
    LeftAlt, RightAlt, LeftSuper, RightSuper,
    
    // Punctuation
    Minus, Equals, LeftBracket, RightBracket,
    Backslash, Semicolon, Apostrophe, Grave,
    Comma, Period, Slash,
    
    // Numpad
    NumPad0, NumPad1, NumPad2, NumPad3, NumPad4,
    NumPad5, NumPad6, NumPad7, NumPad8, NumPad9,
    NumPadDecimal, NumPadEnter, NumPadPlus, NumPadMinus,
    NumPadMultiply, NumPadDivide, NumLock,
    
    // Media
    MediaPlayPause, MediaStop, MediaPrevious, MediaNext,
    VolumeUp, VolumeDown, VolumeMute,
    
    // Misc
    PrintScreen, ScrollLock, Pause
};

//=============================================================================
// Window Events
//=============================================================================

/**
 * @brief Window event types
 */
enum class WindowEventType : u8 {
    Close,
    Resize,
    Move,
    Focus,
    Blur,
    Minimize,
    Maximize,
    Restore,
    DpiChange
};

/**
 * @brief Mouse event types
 */
enum class MouseEventType : u8 {
    Move,
    ButtonDown,
    ButtonUp,
    DoubleClick,
    Wheel,
    Enter,
    Leave
};

/**
 * @brief Keyboard event types
 */
enum class KeyEventType : u8 {
    KeyDown,
    KeyUp,
    KeyRepeat,
    Character  // Text input
};

/**
 * @brief Mouse event data
 */
struct MouseEvent {
    MouseEventType type;
    MouseButton button = MouseButton::None;
    ModifierFlags modifiers = ModifierFlags::None;
    i32 x = 0;              ///< X position relative to window
    i32 y = 0;              ///< Y position relative to window
    i32 globalX = 0;        ///< X position on screen
    i32 globalY = 0;        ///< Y position on screen
    f32 wheelDeltaX = 0.0f; ///< Horizontal scroll
    f32 wheelDeltaY = 0.0f; ///< Vertical scroll
    u32 clickCount = 0;     ///< For multi-click detection
};

/**
 * @brief Keyboard event data
 */
struct KeyEvent {
    KeyEventType type;
    KeyCode keyCode = KeyCode::Unknown;
    ModifierFlags modifiers = ModifierFlags::None;
    u32 nativeKeyCode = 0;  ///< Platform-specific key code
    char32_t character = 0; ///< Unicode character (for Character events)
    bool isRepeat = false;
};

/**
 * @brief Window event data
 */
struct WindowEvent {
    WindowEventType type;
    i32 x = 0;
    i32 y = 0;
    u32 width = 0;
    u32 height = 0;
    f32 dpiScale = 1.0f;
};

//=============================================================================
// Window Configuration
//=============================================================================

/**
 * @brief Window style flags
 */
enum class WindowStyle : u32 {
    None        = 0,
    Titled      = 1 << 0,  ///< Has title bar
    Closable    = 1 << 1,  ///< Has close button
    Minimizable = 1 << 2,  ///< Has minimize button
    Maximizable = 1 << 3,  ///< Has maximize button
    Resizable   = 1 << 4,  ///< Can be resized
    Borderless  = 1 << 5,  ///< No window decoration
    
    // Common combinations
    Default = Titled | Closable | Minimizable | Maximizable | Resizable,
    Dialog = Titled | Closable,
    Tool = Titled | Closable | Resizable
};

inline WindowStyle operator|(WindowStyle a, WindowStyle b) {
    return static_cast<WindowStyle>(static_cast<u32>(a) | static_cast<u32>(b));
}

/**
 * @brief Window configuration
 */
struct WindowConfig {
    std::string title = "Nomad DAW";
    i32 x = -1;             ///< -1 = center
    i32 y = -1;
    u32 width = 1280;
    u32 height = 720;
    u32 minWidth = 800;
    u32 minHeight = 600;
    u32 maxWidth = 0;       ///< 0 = no limit
    u32 maxHeight = 0;
    WindowStyle style = WindowStyle::Default;
    bool visible = true;
    bool maximized = false;
    bool fullscreen = false;
    bool vsync = true;
    bool highDpi = true;
};

//=============================================================================
// Cursor Types
//=============================================================================

/**
 * @brief Standard cursor shapes
 */
enum class CursorType : u8 {
    Arrow,
    IBeam,
    Crosshair,
    Hand,
    ResizeNS,
    ResizeEW,
    ResizeNWSE,
    ResizeNESW,
    ResizeAll,
    NotAllowed,
    Wait,
    Hidden
};

//=============================================================================
// Window Interface
//=============================================================================

/**
 * @brief Window event callback types
 */
using WindowEventCallback = std::function<void(const WindowEvent&)>;
using MouseEventCallback = std::function<void(const MouseEvent&)>;
using KeyEventCallback = std::function<void(const KeyEvent&)>;
using FileDropCallback = std::function<void(const std::vector<std::string>&)>;

/**
 * @brief Window interface
 * 
 * Represents a native window with input handling and rendering context.
 */
class IWindow {
public:
    virtual ~IWindow() = default;
    
    //=========================================================================
    // Window Management
    //=========================================================================
    
    /**
     * @brief Show window
     */
    virtual void show() = 0;
    
    /**
     * @brief Hide window
     */
    virtual void hide() = 0;
    
    /**
     * @brief Close window
     */
    virtual void close() = 0;
    
    /**
     * @brief Minimize window
     */
    virtual void minimize() = 0;
    
    /**
     * @brief Maximize window
     */
    virtual void maximize() = 0;
    
    /**
     * @brief Restore from minimized/maximized state
     */
    virtual void restore() = 0;
    
    /**
     * @brief Set fullscreen mode
     */
    virtual void setFullscreen(bool fullscreen) = 0;
    
    /**
     * @brief Bring window to front and focus
     */
    virtual void focus() = 0;
    
    /**
     * @brief Request window repaint
     */
    virtual void invalidate() = 0;
    
    //=========================================================================
    // Properties
    //=========================================================================
    
    /**
     * @brief Set window title
     */
    virtual void setTitle(std::string_view title) = 0;
    
    /**
     * @brief Get window title
     */
    [[nodiscard]] virtual std::string getTitle() const = 0;
    
    /**
     * @brief Set window position
     */
    virtual void setPosition(i32 x, i32 y) = 0;
    
    /**
     * @brief Get window position
     */
    virtual void getPosition(i32& x, i32& y) const = 0;
    
    /**
     * @brief Set window size
     */
    virtual void setSize(u32 width, u32 height) = 0;
    
    /**
     * @brief Get window size
     */
    virtual void getSize(u32& width, u32& height) const = 0;
    
    /**
     * @brief Get client area size (excluding decorations)
     */
    virtual void getClientSize(u32& width, u32& height) const = 0;
    
    /**
     * @brief Get DPI scale factor
     */
    [[nodiscard]] virtual f32 getDpiScale() const = 0;
    
    /**
     * @brief Check if window is visible
     */
    [[nodiscard]] virtual bool isVisible() const = 0;
    
    /**
     * @brief Check if window is focused
     */
    [[nodiscard]] virtual bool isFocused() const = 0;
    
    /**
     * @brief Check if window is minimized
     */
    [[nodiscard]] virtual bool isMinimized() const = 0;
    
    /**
     * @brief Check if window is maximized
     */
    [[nodiscard]] virtual bool isMaximized() const = 0;
    
    /**
     * @brief Check if window is fullscreen
     */
    [[nodiscard]] virtual bool isFullscreen() const = 0;
    
    //=========================================================================
    // Cursor
    //=========================================================================
    
    /**
     * @brief Set cursor type
     */
    virtual void setCursor(CursorType cursor) = 0;
    
    /**
     * @brief Capture mouse (for dragging)
     */
    virtual void captureMouse() = 0;
    
    /**
     * @brief Release mouse capture
     */
    virtual void releaseMouse() = 0;
    
    /**
     * @brief Set mouse position relative to window
     */
    virtual void setMousePosition(i32 x, i32 y) = 0;
    
    //=========================================================================
    // Events
    //=========================================================================
    
    /**
     * @brief Set window event callback
     */
    virtual void setWindowEventCallback(WindowEventCallback callback) = 0;
    
    /**
     * @brief Set mouse event callback
     */
    virtual void setMouseEventCallback(MouseEventCallback callback) = 0;
    
    /**
     * @brief Set keyboard event callback
     */
    virtual void setKeyEventCallback(KeyEventCallback callback) = 0;
    
    /**
     * @brief Set file drop callback
     */
    virtual void setFileDropCallback(FileDropCallback callback) = 0;
    
    /**
     * @brief Process pending events
     */
    virtual void pollEvents() = 0;
    
    //=========================================================================
    // Rendering
    //=========================================================================
    
    /**
     * @brief Make OpenGL context current (if using OpenGL)
     */
    virtual void makeContextCurrent() = 0;
    
    /**
     * @brief Swap buffers (present frame)
     */
    virtual void swapBuffers() = 0;
    
    /**
     * @brief Set VSync
     */
    virtual void setVSync(bool enabled) = 0;
    
    /**
     * @brief Get native window handle
     * - Windows: HWND
     * - macOS: NSWindow*
     * - Linux: Window (X11) or wl_surface* (Wayland)
     */
    [[nodiscard]] virtual void* getNativeHandle() const = 0;
};

/**
 * @brief Create a window
 * @param config Window configuration
 * @return Window instance
 */
[[nodiscard]] std::unique_ptr<IWindow> createWindow(const WindowConfig& config);

/**
 * @brief Check if main event loop should continue
 */
[[nodiscard]] bool shouldQuit();

/**
 * @brief Request application quit
 */
void requestQuit();

} // namespace nomad::platform
