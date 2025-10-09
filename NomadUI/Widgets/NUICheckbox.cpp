#include "NUICheckbox.h"
#include "../Core/NUITheme.h"

namespace NomadUI {

NUICheckbox::NUICheckbox() : NUICheckbox("", false) {
}

NUICheckbox::NUICheckbox(const std::string& label, bool checked)
    : checked_(checked), label_(label), checkAlpha_(checked ? 1.0f : 0.0f) {
}

void NUICheckbox::setChecked(bool checked) {
    if (checked_ != checked) {
        checked_ = checked;
        setDirty();
        
        if (onChange_) {
            onChange_(checked_);
        }
    }
}

void NUICheckbox::toggle() {
    setChecked(!checked_);
}

void NUICheckbox::setLabel(const std::string& label) {
    if (label_ != label) {
        label_ = label;
        setDirty();
    }
}

void NUICheckbox::onRender(NUIRenderer& renderer) {
    auto theme = getTheme();
    if (!theme) {
        return;
    }
    
    auto bounds = getBounds();
    auto boxBounds = getBoxBounds();
    
    // Draw glow effect when hovered
    if (hoverAlpha_ > 0.01f) {
        renderer.drawGlow(
            boxBounds,
            boxSize_ * 0.8f,
            hoverAlpha_ * theme->getGlowIntensity(),
            theme->getPrimary()
        );
    }
    
    // Draw checkbox box
    NUIColor boxColor = getCurrentBoxColor();
    renderer.fillRoundedRect(boxBounds, theme->getBorderRadius() * 0.5f, boxColor);
    
    // Draw border
    NUIColor borderColor = checked_ || isHovered() 
        ? theme->getPrimary() 
        : theme->getBorder();
    
    renderer.strokeRoundedRect(
        boxBounds,
        theme->getBorderRadius() * 0.5f,
        theme->getBorderWidth(),
        borderColor
    );
    
    // Draw checkmark with animation
    if (checkAlpha_ > 0.01f) {
        NUIColor checkColor = getCurrentCheckColor().withAlpha(checkAlpha_);
        
        // Draw checkmark symbol
        float padding = boxSize_ * 0.25f;
        float centerX = boxBounds.x + boxBounds.width * 0.5f;
        float centerY = boxBounds.y + boxBounds.height * 0.5f;
        
        // Simple checkmark using filled rect (in a real implementation, use path rendering)
        float checkSize = boxSize_ * 0.5f * checkAlpha_;
        NUIRect checkRect = {
            centerX - checkSize * 0.5f,
            centerY - checkSize * 0.5f,
            checkSize,
            checkSize
        };
        renderer.fillRoundedRect(checkRect, checkSize * 0.2f, checkColor);
    }
    
    // Draw label
    if (!label_.empty()) {
        float labelX = boxBounds.right() + theme->getPadding();
        float labelY = bounds.y + (bounds.height - theme->getFontSizeNormal()) * 0.5f;
        
        renderer.drawText(
            label_,
            NUIPoint{labelX, labelY},
            theme->getFontSizeNormal(),
            getCurrentLabelColor()
        );
    }
    
    // Render children
    NUIComponent::onRender(renderer);
}

void NUICheckbox::onUpdate(double deltaTime) {
    // Animate hover effect
    float targetHoverAlpha = isHovered() ? 1.0f : 0.0f;
    float speed = 8.0f;
    
    if (hoverAlpha_ < targetHoverAlpha) {
        hoverAlpha_ += speed * deltaTime;
        if (hoverAlpha_ > targetHoverAlpha) {
            hoverAlpha_ = targetHoverAlpha;
        }
        setDirty();
    } else if (hoverAlpha_ > targetHoverAlpha) {
        hoverAlpha_ -= speed * deltaTime;
        if (hoverAlpha_ < targetHoverAlpha) {
            hoverAlpha_ = targetHoverAlpha;
        }
        setDirty();
    }
    
    // Animate check state
    float targetCheckAlpha = checked_ ? 1.0f : 0.0f;
    float checkSpeed = 10.0f;
    
    if (checkAlpha_ < targetCheckAlpha) {
        checkAlpha_ += checkSpeed * deltaTime;
        if (checkAlpha_ > targetCheckAlpha) {
            checkAlpha_ = targetCheckAlpha;
        }
        setDirty();
    } else if (checkAlpha_ > targetCheckAlpha) {
        checkAlpha_ -= checkSpeed * deltaTime;
        if (checkAlpha_ < targetCheckAlpha) {
            checkAlpha_ = targetCheckAlpha;
        }
        setDirty();
    }
    
    NUIComponent::onUpdate(deltaTime);
}

bool NUICheckbox::onMouseEvent(const NUIMouseEvent& event) {
    if (!isEnabled()) {
        return false;
    }
    
    auto boxBounds = getBoxBounds();
    
    // Handle mouse down
    if (event.pressed && event.button == NUIMouseButton::Left) {
        if (boxBounds.contains(event.position) || 
            (!label_.empty() && containsPoint(event.position))) {
            pressed_ = true;
            setDirty();
            return true;
        }
    }
    
    // Handle mouse up (toggle)
    if (event.released && event.button == NUIMouseButton::Left) {
        if (pressed_) {
            pressed_ = false;
            
            if (boxBounds.contains(event.position) || 
                (!label_.empty() && containsPoint(event.position))) {
                toggle();
            }
            
            setDirty();
            return true;
        }
    }
    
    return false;
}

NUIRect NUICheckbox::getBoxBounds() const {
    auto bounds = getBounds();
    return NUIRect{
        bounds.x,
        bounds.y + (bounds.height - boxSize_) * 0.5f,
        boxSize_,
        boxSize_
    };
}

NUIColor NUICheckbox::getCurrentBoxColor() const {
    auto theme = getTheme();
    
    if (useCustomColors_) {
        return boxColor_;
    }
    
    if (!theme) {
        return NUIColor::fromHex(0x333333);
    }
    
    if (!isEnabled()) {
        return theme->getDisabled();
    }
    
    if (pressed_) {
        return theme->getActive();
    }
    
    if (checked_) {
        return theme->getPrimary().withBrightness(0.3f);
    }
    
    return theme->getSurface();
}

NUIColor NUICheckbox::getCurrentCheckColor() const {
    auto theme = getTheme();
    
    if (useCustomColors_) {
        return checkColor_;
    }
    
    if (!theme) {
        return NUIColor::white();
    }
    
    return theme->getPrimary();
}

NUIColor NUICheckbox::getCurrentLabelColor() const {
    auto theme = getTheme();
    
    if (useCustomColors_) {
        return labelColor_;
    }
    
    if (!theme) {
        return NUIColor::white();
    }
    
    if (!isEnabled()) {
        return theme->getColor("textDisabled");
    }
    
    return theme->getText();
}

} // namespace NomadUI
