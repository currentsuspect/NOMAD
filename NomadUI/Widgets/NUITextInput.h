#pragma once

#include "../Core/NUIComponent.h"
#include "../Graphics/NUIRenderer.h"
#include <string>
#include <functional>

namespace NomadUI {

/**
 * Text Input widget for Nomad UI.
 * 
 * Features:
 * - Text entry and editing
 * - Cursor with blinking animation
 * - Text selection (basic)
 * - Placeholder text
 * - Change callback
 * - Password mode
 */
class NUITextInput : public NUIComponent {
public:
    NUITextInput();
    NUITextInput(const std::string& placeholder);
    ~NUITextInput() override = default;
    
    // ========================================================================
    // Configuration
    // ========================================================================
    
    /**
     * Set the input text.
     */
    void setText(const std::string& text);
    std::string getText() const { return text_; }
    
    /**
     * Set placeholder text.
     */
    void setPlaceholder(const std::string& placeholder);
    std::string getPlaceholder() const { return placeholder_; }
    
    /**
     * Set text change callback.
     */
    void setOnTextChange(std::function<void(const std::string&)> callback) { onTextChange_ = callback; }
    
    /**
     * Set submit callback (Enter key).
     */
    void setOnSubmit(std::function<void(const std::string&)> callback) { onSubmit_ = callback; }
    
    /**
     * Enable/disable password mode.
     */
    void setPasswordMode(bool enabled) { passwordMode_ = enabled; setDirty(); }
    bool isPasswordMode() const { return passwordMode_; }
    
    /**
     * Set custom colors (overrides theme).
     */
    void setBackgroundColor(const NUIColor& color) { backgroundColor_ = color; useCustomColors_ = true; setDirty(); }
    void setTextColor(const NUIColor& color) { textColor_ = color; useCustomColors_ = true; setDirty(); }
    void setPlaceholderColor(const NUIColor& color) { placeholderColor_ = color; useCustomColors_ = true; setDirty(); }
    
    /**
     * Reset to theme colors.
     */
    void resetColors() { useCustomColors_ = false; setDirty(); }
    
    // ========================================================================
    // Component Overrides
    // ========================================================================
    
    void onRender(NUIRenderer& renderer) override;
    void onUpdate(double deltaTime) override;
    bool onMouseEvent(const NUIMouseEvent& event) override;
    bool onKeyEvent(const NUIKeyEvent& event) override;
    void onFocusGained() override;
    void onFocusLost() override;
    
private:
    std::string text_;
    std::string placeholder_;
    size_t cursorPos_ = 0;
    bool passwordMode_ = false;
    
    bool useCustomColors_ = false;
    NUIColor backgroundColor_;
    NUIColor textColor_;
    NUIColor placeholderColor_;
    
    float cursorBlinkTime_ = 0.0f;
    bool cursorVisible_ = true;
    float hoverAlpha_ = 0.0f;
    
    std::function<void(const std::string&)> onTextChange_;
    std::function<void(const std::string&)> onSubmit_;
    
    std::string getDisplayText() const;
    NUIColor getCurrentBackgroundColor() const;
    NUIColor getCurrentTextColor() const;
    NUIColor getCurrentPlaceholderColor() const;
};

} // namespace NomadUI
