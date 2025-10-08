#pragma once

#include <cstdint>
#include <string>
#include <memory>
#include <functional>

namespace NomadUI {

// ============================================================================
// Basic Types
// ============================================================================

struct NUIPoint {
    float x = 0.0f;
    float y = 0.0f;
    
    NUIPoint() = default;
    NUIPoint(float x, float y) : x(x), y(y) {}
};

struct NUISize {
    float width = 0.0f;
    float height = 0.0f;
    
    NUISize() = default;
    NUISize(float w, float h) : width(w), height(h) {}
};

struct NUIRect {
    float x = 0.0f;
    float y = 0.0f;
    float width = 0.0f;
    float height = 0.0f;
    
    NUIRect() = default;
    NUIRect(float x, float y, float w, float h) 
        : x(x), y(y), width(w), height(h) {}
    
    bool contains(float px, float py) const {
        return px >= x && px < x + width &&
               py >= y && py < y + height;
    }
    
    bool contains(const NUIPoint& p) const {
        return contains(p.x, p.y);
    }
    
    float right() const { return x + width; }
    float bottom() const { return y + height; }
    NUIPoint center() const { return {x + width * 0.5f, y + height * 0.5f}; }
};

struct NUIColor {
    float r = 0.0f;
    float g = 0.0f;
    float b = 0.0f;
    float a = 1.0f;
    
    NUIColor() = default;
    NUIColor(float r, float g, float b, float a = 1.0f)
        : r(r), g(g), b(b), a(a) {}
    
    // Create from hex (e.g., 0xa855f7)
    static NUIColor fromHex(uint32_t hex, float alpha = 1.0f) {
        return NUIColor(
            ((hex >> 16) & 0xFF) / 255.0f,
            ((hex >> 8) & 0xFF) / 255.0f,
            (hex & 0xFF) / 255.0f,
            alpha
        );
    }
    
    // Utility colors
    static NUIColor white() { return {1.0f, 1.0f, 1.0f, 1.0f}; }
    static NUIColor black() { return {0.0f, 0.0f, 0.0f, 1.0f}; }
    static NUIColor transparent() { return {0.0f, 0.0f, 0.0f, 0.0f}; }
    
    NUIColor withAlpha(float newAlpha) const {
        return {r, g, b, newAlpha};
    }
    
    NUIColor withBrightness(float factor) const {
        return {r * factor, g * factor, b * factor, a};
    }
};

// ============================================================================
// Enums
// ============================================================================

enum class NUIMouseButton {
    None,
    Left,
    Right,
    Middle
};

enum class NUIKeyCode {
    Unknown,
    Space, Enter, Escape, Tab, Backspace, Delete,
    Left, Right, Up, Down,
    A, B, C, D, E, F, G, H, I, J, K, L, M,
    N, O, P, Q, R, S, T, U, V, W, X, Y, Z,
    Num0, Num1, Num2, Num3, Num4, Num5, Num6, Num7, Num8, Num9,
    F1, F2, F3, F4, F5, F6, F7, F8, F9, F10, F11, F12
};

enum class NUIModifiers {
    None = 0,
    Shift = 1 << 0,
    Ctrl = 1 << 1,
    Alt = 1 << 2,
    Super = 1 << 3  // Windows key / Command key
};

inline NUIModifiers operator|(NUIModifiers a, NUIModifiers b) {
    return static_cast<NUIModifiers>(static_cast<int>(a) | static_cast<int>(b));
}

inline bool operator&(NUIModifiers a, NUIModifiers b) {
    return (static_cast<int>(a) & static_cast<int>(b)) != 0;
}

enum class NUIEasing {
    Linear,
    EaseIn,
    EaseOut,
    EaseInOut,
    BounceIn,
    BounceOut,
    ElasticIn,
    ElasticOut,
    BackIn,
    BackOut
};

enum class NUIAlignment {
    Start,
    Center,
    End,
    Stretch
};

enum class NUIDirection {
    Horizontal,
    Vertical
};

// ============================================================================
// Event Structures
// ============================================================================

struct NUIMouseEvent {
    NUIPoint position;
    NUIPoint delta;  // For drag events
    NUIMouseButton button = NUIMouseButton::None;
    NUIModifiers modifiers = NUIModifiers::None;
    float wheelDelta = 0.0f;
    bool pressed = false;
    bool released = false;
    bool doubleClick = false;
};

struct NUIKeyEvent {
    NUIKeyCode keyCode = NUIKeyCode::Unknown;
    NUIModifiers modifiers = NUIModifiers::None;
    char character = 0;  // For text input
    bool pressed = false;
    bool released = false;
    bool repeat = false;
};

struct NUIResizeEvent {
    int width = 0;
    int height = 0;
};

// ============================================================================
// Callbacks
// ============================================================================

using NUIMouseCallback = std::function<void(const NUIMouseEvent&)>;
using NUIKeyCallback = std::function<void(const NUIKeyEvent&)>;
using NUIResizeCallback = std::function<void(const NUIResizeEvent&)>;
using NUIUpdateCallback = std::function<void(double deltaTime)>;
using NUIRenderCallback = std::function<void()>;

} // namespace NomadUI
