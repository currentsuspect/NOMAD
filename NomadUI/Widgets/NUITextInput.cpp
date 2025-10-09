#include "NUITextInput.h"
#include "../Core/NUITheme.h"

namespace NomadUI {

NUITextInput::NUITextInput() : NUITextInput("") {
}

NUITextInput::NUITextInput(const std::string& placeholder)
    : placeholder_(placeholder) {
}

void NUITextInput::setText(const std::string& text) {
    if (text_ != text) {
        text_ = text;
        cursorPos_ = text_.length();
        setDirty();
        
        if (onTextChange_) {
            onTextChange_(text_);
        }
    }
}

void NUITextInput::setPlaceholder(const std::string& placeholder) {
    if (placeholder_ != placeholder) {
        placeholder_ = placeholder;
        setDirty();
    }
}

void NUITextInput::onRender(NUIRenderer& renderer) {
    auto theme = getTheme();
    if (!theme) {
        return;
    }
    
    auto bounds = getBounds();
    float radius = theme->getBorderRadius();
    
    // Draw glow effect when focused
    if (isFocused()) {
        renderer.drawGlow(
            bounds,
            radius * 2.0f,
            theme->getGlowIntensity(),
            theme->getPrimary()
        );
    } else if (hoverAlpha_ > 0.01f) {
        renderer.drawGlow(
            bounds,
            radius * 2.0f,
            hoverAlpha_ * theme->getGlowIntensity() * 0.5f,
            theme->getPrimary()
        );
    }
    
    // Draw background
    renderer.fillRoundedRect(bounds, radius, getCurrentBackgroundColor());
    
    // Draw border
    NUIColor borderColor = isFocused() 
        ? theme->getPrimary() 
        : (isHovered() ? theme->getPrimary().withBrightness(0.7f) : theme->getBorder());
    
    renderer.strokeRoundedRect(
        bounds,
        radius,
        theme->getBorderWidth(),
        borderColor
    );
    
    // Draw text or placeholder
    float padding = theme->getPadding();
    NUIPoint textPos = {bounds.x + padding, bounds.y + (bounds.height - theme->getFontSizeNormal()) * 0.5f};
    
    if (text_.empty()) {
        // Draw placeholder
        renderer.drawText(
            placeholder_,
            textPos,
            theme->getFontSizeNormal(),
            getCurrentPlaceholderColor()
        );
    } else {
        // Draw text
        std::string displayText = getDisplayText();
        renderer.drawText(
            displayText,
            textPos,
            theme->getFontSizeNormal(),
            getCurrentTextColor()
        );
        
        // Draw cursor when focused
        if (isFocused() && cursorVisible_) {
            // Simplified cursor - just a vertical line at the end of text
            // In a full implementation, this would calculate position based on cursorPos_
            float cursorX = textPos.x + displayText.length() * 8.0f; // Rough approximation
            renderer.fillRect(
                NUIRect{cursorX, textPos.y, 2.0f, theme->getFontSizeNormal()},
                theme->getPrimary()
            );
        }
    }
    
    // Render children
    NUIComponent::onRender(renderer);
}

void NUITextInput::onUpdate(double deltaTime) {
    // Animate hover effect
    float targetAlpha = isHovered() ? 1.0f : 0.0f;
    float speed = 8.0f;
    
    if (hoverAlpha_ < targetAlpha) {
        hoverAlpha_ += speed * deltaTime;
        if (hoverAlpha_ > targetAlpha) {
            hoverAlpha_ = targetAlpha;
        }
        setDirty();
    } else if (hoverAlpha_ > targetAlpha) {
        hoverAlpha_ -= speed * deltaTime;
        if (hoverAlpha_ < targetAlpha) {
            hoverAlpha_ = targetAlpha;
        }
        setDirty();
    }
    
    // Blink cursor
    if (isFocused()) {
        cursorBlinkTime_ += deltaTime;
        if (cursorBlinkTime_ >= 0.5f) {
            cursorBlinkTime_ = 0.0f;
            cursorVisible_ = !cursorVisible_;
            setDirty();
        }
    }
    
    NUIComponent::onUpdate(deltaTime);
}

bool NUITextInput::onMouseEvent(const NUIMouseEvent& event) {
    if (!isEnabled()) {
        return false;
    }
    
    // Focus on click
    if (event.pressed && event.button == NUIMouseButton::Left) {
        if (containsPoint(event.position)) {
            setFocused(true);
            return true;
        }
    }
    
    return false;
}

bool NUITextInput::onKeyEvent(const NUIKeyEvent& event) {
    if (!isFocused() || !event.pressed) {
        return false;
    }
    
    bool handled = false;
    
    // Handle backspace
    if (event.keyCode == NUIKeyCode::Backspace) {
        if (!text_.empty() && cursorPos_ > 0) {
            text_.erase(cursorPos_ - 1, 1);
            cursorPos_--;
            setDirty();
            
            if (onTextChange_) {
                onTextChange_(text_);
            }
        }
        handled = true;
    }
    // Handle delete
    else if (event.keyCode == NUIKeyCode::Delete) {
        if (cursorPos_ < text_.length()) {
            text_.erase(cursorPos_, 1);
            setDirty();
            
            if (onTextChange_) {
                onTextChange_(text_);
            }
        }
        handled = true;
    }
    // Handle enter (submit)
    else if (event.keyCode == NUIKeyCode::Enter) {
        if (onSubmit_) {
            onSubmit_(text_);
        }
        handled = true;
    }
    // Handle left arrow
    else if (event.keyCode == NUIKeyCode::Left) {
        if (cursorPos_ > 0) {
            cursorPos_--;
            setDirty();
        }
        handled = true;
    }
    // Handle right arrow
    else if (event.keyCode == NUIKeyCode::Right) {
        if (cursorPos_ < text_.length()) {
            cursorPos_++;
            setDirty();
        }
        handled = true;
    }
    // Handle character input
    else if (event.character >= 32 && event.character < 127) {
        text_.insert(cursorPos_, 1, event.character);
        cursorPos_++;
        setDirty();
        
        if (onTextChange_) {
            onTextChange_(text_);
        }
        handled = true;
    }
    
    return handled;
}

void NUITextInput::onFocusGained() {
    cursorVisible_ = true;
    cursorBlinkTime_ = 0.0f;
    setDirty();
}

void NUITextInput::onFocusLost() {
    cursorVisible_ = false;
    setDirty();
}

std::string NUITextInput::getDisplayText() const {
    if (passwordMode_) {
        return std::string(text_.length(), '*');
    }
    return text_;
}

NUIColor NUITextInput::getCurrentBackgroundColor() const {
    auto theme = getTheme();
    
    if (useCustomColors_) {
        return backgroundColor_;
    }
    
    if (!theme) {
        return NUIColor::fromHex(0x1a1a1a);
    }
    
    if (!isEnabled()) {
        return theme->getDisabled();
    }
    
    return theme->getSurface();
}

NUIColor NUITextInput::getCurrentTextColor() const {
    auto theme = getTheme();
    
    if (useCustomColors_) {
        return textColor_;
    }
    
    if (!theme) {
        return NUIColor::white();
    }
    
    if (!isEnabled()) {
        return theme->getColor("textDisabled");
    }
    
    return theme->getText();
}

NUIColor NUITextInput::getCurrentPlaceholderColor() const {
    auto theme = getTheme();
    
    if (useCustomColors_) {
        return placeholderColor_;
    }
    
    if (!theme) {
        return NUIColor::white().withAlpha(0.5f);
    }
    
    return theme->getTextSecondary();
}

} // namespace NomadUI
