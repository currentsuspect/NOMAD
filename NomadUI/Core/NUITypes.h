#pragma once

#include <cstdint>
#include <string>
#include <memory>
#include <functional>
#include <cmath>
#include <algorithm>

namespace NomadUI {

// ============================================================================
// Basic Types
// ============================================================================

struct NUIPoint {
    float x = 0.0f;
    float y = 0.0f;
    
    NUIPoint() = default;
    NUIPoint(float x, float y) : x(x), y(y) {}
    
    NUIPoint operator+(const NUIPoint& other) const {
        return NUIPoint(x + other.x, y + other.y);
    }
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
    NUIPoint getCentre() const { return center(); }
    float getWidth() const { return width; }
    float getHeight() const { return height; }
    bool isEmpty() const { return width <= 0.0f || height <= 0.0f; }
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
    
    NUIColor lightened(float factor) const {
        return withBrightness(1.0f + factor);
    }
    
    NUIColor darkened(float factor) const {
        return withBrightness(1.0f - factor);
    }
    
    static NUIColor lerp(const NUIColor& a, const NUIColor& b, float t) {
        return NUIColor(
            a.r + (b.r - a.r) * t,
            a.g + (b.g - a.g) * t,
            a.b + (b.b - a.b) * t,
            a.a + (b.a - a.a) * t
        );
    }
    
    // Modern color palette
    static NUIColor Primary() { return NUIColor(0.2f, 0.4f, 0.8f, 1.0f); }
    static NUIColor Secondary() { return NUIColor(0.6f, 0.6f, 0.6f, 1.0f); }
    static NUIColor Success() { return NUIColor(0.2f, 0.7f, 0.3f, 1.0f); }
    static NUIColor Warning() { return NUIColor(0.9f, 0.6f, 0.1f, 1.0f); }
    static NUIColor Error() { return NUIColor(0.8f, 0.2f, 0.2f, 1.0f); }
    static NUIColor Info() { return NUIColor(0.1f, 0.6f, 0.8f, 1.0f); }
    
    // Dark theme colors
    static NUIColor DarkBackground() { return NUIColor(0.1f, 0.1f, 0.15f, 1.0f); }
    static NUIColor DarkSurface() { return NUIColor(0.15f, 0.15f, 0.2f, 1.0f); }
    static NUIColor DarkBorder() { return NUIColor(0.3f, 0.3f, 0.35f, 1.0f); }
    static NUIColor DarkText() { return NUIColor(0.9f, 0.9f, 0.9f, 1.0f); }
    static NUIColor DarkTextSecondary() { return NUIColor(0.6f, 0.6f, 0.6f, 1.0f); }
    
    // Light theme colors
    static NUIColor LightBackground() { return NUIColor(0.98f, 0.98f, 0.98f, 1.0f); }
    static NUIColor LightSurface() { return NUIColor(0.95f, 0.95f, 0.95f, 1.0f); }
    static NUIColor LightBorder() { return NUIColor(0.8f, 0.8f, 0.8f, 1.0f); }
    static NUIColor LightText() { return NUIColor(0.1f, 0.1f, 0.1f, 1.0f); }
    static NUIColor LightTextSecondary() { return NUIColor(0.4f, 0.4f, 0.4f, 1.0f); }
    
    // HSL color space support
    struct HSL {
        float h = 0.0f; // Hue (0-360)
        float s = 0.0f; // Saturation (0-1)
        float l = 0.0f; // Lightness (0-1)
        float a = 1.0f; // Alpha
        
        HSL() = default;
        HSL(float h, float s, float l, float a = 1.0f) : h(h), s(s), l(l), a(a) {}
    };
    
    // Convert RGB to HSL
    HSL toHSL() const {
        float max = std::max({r, g, b});
        float min = std::min({r, g, b});
        float delta = max - min;
        
        float h = 0.0f;
        if (delta != 0.0f) {
            if (max == r) {
                h = 60.0f * fmodf((g - b) / delta, 6.0f);
            } else if (max == g) {
                h = 60.0f * ((b - r) / delta + 2.0f);
            } else {
                h = 60.0f * ((r - g) / delta + 4.0f);
            }
        }
        if (h < 0.0f) h += 360.0f;
        
        float l = (max + min) / 2.0f;
        float s = (delta == 0.0f) ? 0.0f : delta / (1.0f - std::abs(2.0f * l - 1.0f));
        
        return HSL(h, s, l, a);
    }
    
    // Convert HSL to RGB
    static NUIColor fromHSL(const HSL& hsl) {
        float c = (1.0f - std::abs(2.0f * hsl.l - 1.0f)) * hsl.s;
        float x = c * (1.0f - std::abs(fmodf(hsl.h / 60.0f, 2.0f) - 1.0f));
        float m = hsl.l - c / 2.0f;
        
        float r, g, b;
        if (hsl.h < 60.0f) {
            r = c; g = x; b = 0.0f;
        } else if (hsl.h < 120.0f) {
            r = x; g = c; b = 0.0f;
        } else if (hsl.h < 180.0f) {
            r = 0.0f; g = c; b = x;
        } else if (hsl.h < 240.0f) {
            r = 0.0f; g = x; b = c;
        } else if (hsl.h < 300.0f) {
            r = x; g = 0.0f; b = c;
        } else {
            r = c; g = 0.0f; b = x;
        }
        
        return NUIColor(r + m, g + m, b + m, hsl.a);
    }
    
    // HSL manipulation
    NUIColor withHue(float hue) const {
        HSL hsl = toHSL();
        hsl.h = fmodf(hue, 360.0f);
        return fromHSL(hsl);
    }
    
    NUIColor withSaturation(float saturation) const {
        HSL hsl = toHSL();
        hsl.s = std::clamp(saturation, 0.0f, 1.0f);
        return fromHSL(hsl);
    }
    
    NUIColor withLightness(float lightness) const {
        HSL hsl = toHSL();
        hsl.l = std::clamp(lightness, 0.0f, 1.0f);
        return fromHSL(hsl);
    }
    
    // Advanced color operations
    NUIColor withContrast(float contrast) const {
        HSL hsl = toHSL();
        hsl.l = std::clamp(hsl.l * contrast, 0.0f, 1.0f);
        return fromHSL(hsl);
    }
    
    NUIColor withVibrance(float vibrance) const {
        HSL hsl = toHSL();
        hsl.s = std::clamp(hsl.s * vibrance, 0.0f, 1.0f);
        return fromHSL(hsl);
    }
    
    NUIColor complementary() const {
        HSL hsl = toHSL();
        hsl.h = fmodf(hsl.h + 180.0f, 360.0f);
        return fromHSL(hsl);
    }
    
    NUIColor triadic1() const {
        HSL hsl = toHSL();
        hsl.h = fmodf(hsl.h + 120.0f, 360.0f);
        return fromHSL(hsl);
    }
    
    NUIColor triadic2() const {
        HSL hsl = toHSL();
        hsl.h = fmodf(hsl.h + 240.0f, 360.0f);
        return fromHSL(hsl);
    }
    
    // Utility functions
    float luminance() const {
        return 0.299f * r + 0.587f * g + 0.114f * b;
    }
    
    bool isDark() const {
        return luminance() < 0.5f;
    }
    
    NUIColor textColor() const {
        return isDark() ? LightText() : DarkText();
    }
    
    // Hex color support
    uint32_t toHex() const {
        uint32_t r8 = static_cast<uint32_t>(r * 255.0f) & 0xFF;
        uint32_t g8 = static_cast<uint32_t>(g * 255.0f) & 0xFF;
        uint32_t b8 = static_cast<uint32_t>(b * 255.0f) & 0xFF;
        uint32_t a8 = static_cast<uint32_t>(a * 255.0f) & 0xFF;
        return (a8 << 24) | (r8 << 16) | (g8 << 8) | b8;
    }
    
    // Smooth color transitions
    static NUIColor lerpHSL(const NUIColor& a, const NUIColor& b, float t) {
        HSL hslA = a.toHSL();
        HSL hslB = b.toHSL();
        
        // Handle hue wrapping
        float h = hslA.h;
        float hDiff = hslB.h - hslA.h;
        if (hDiff > 180.0f) hDiff -= 360.0f;
        else if (hDiff < -180.0f) hDiff += 360.0f;
        h += hDiff * t;
        if (h < 0.0f) h += 360.0f;
        if (h >= 360.0f) h -= 360.0f;
        
        return fromHSL(HSL(
            h,
            hslA.s + (hslB.s - hslA.s) * t,
            hslA.l + (hslB.l - hslA.l) * t,
            hslA.a + (hslB.a - hslA.a) * t
        ));
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

enum class NUITextAlignment {
    Left,
    Center,
    Right,
    Justified
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
