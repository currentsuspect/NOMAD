#pragma once

#include "../Core/NUIComponent.h"
#include "../Graphics/NUIRenderer.h"
#include <string>

namespace NomadUI {

/**
 * Label widget for Nomad UI.
 * 
 * Features:
 * - Simple text display
 * - Customizable alignment
 * - Word wrapping
 * - Color customization
 * - Shadow/outline effects
 */
class NUILabel : public NUIComponent {
public:
    enum class TextAlign {
        Left,
        Center,
        Right
    };
    
    enum class VerticalAlign {
        Top,
        Middle,
        Bottom
    };
    
    NUILabel();
    NUILabel(const std::string& text);
    ~NUILabel() override = default;
    
    // ========================================================================
    // Configuration
    // ========================================================================
    
    /**
     * Set the label text.
     */
    void setText(const std::string& text);
    std::string getText() const { return text_; }
    
    /**
     * Set text alignment.
     */
    void setTextAlign(TextAlign align) { textAlign_ = align; setDirty(); }
    TextAlign getTextAlign() const { return textAlign_; }
    
    /**
     * Set vertical alignment.
     */
    void setVerticalAlign(VerticalAlign align) { verticalAlign_ = align; setDirty(); }
    VerticalAlign getVerticalAlign() const { return verticalAlign_; }
    
    /**
     * Set text color (overrides theme).
     */
    void setTextColor(const NUIColor& color) { textColor_ = color; useCustomColor_ = true; setDirty(); }
    
    /**
     * Reset to theme color.
     */
    void resetColor() { useCustomColor_ = false; setDirty(); }
    
    /**
     * Set font size.
     */
    void setFontSize(float size) { fontSize_ = size; setDirty(); }
    float getFontSize() const { return fontSize_; }
    
    /**
     * Enable/disable word wrapping.
     */
    void setWordWrap(bool enable) { wordWrap_ = enable; setDirty(); }
    bool isWordWrap() const { return wordWrap_; }
    
    /**
     * Enable/disable shadow effect.
     */
    void setShadowEnabled(bool enabled) { shadowEnabled_ = enabled; setDirty(); }
    bool isShadowEnabled() const { return shadowEnabled_; }
    
    // ========================================================================
    // Component Overrides
    // ========================================================================
    
    void onRender(NUIRenderer& renderer) override;
    
private:
    std::string text_;
    TextAlign textAlign_ = TextAlign::Left;
    VerticalAlign verticalAlign_ = VerticalAlign::Top;
    float fontSize_ = 0.0f; // 0 = use theme default
    bool wordWrap_ = false;
    bool shadowEnabled_ = false;
    
    bool useCustomColor_ = false;
    NUIColor textColor_;
    
    NUIColor getCurrentTextColor() const;
};

} // namespace NomadUI
