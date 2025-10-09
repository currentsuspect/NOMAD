#include "NUILabel.h"
#include "../Core/NUITheme.h"

namespace NomadUI {

NUILabel::NUILabel() : NUILabel("Label") {
}

NUILabel::NUILabel(const std::string& text) : text_(text) {
}

void NUILabel::setText(const std::string& text) {
    if (text_ != text) {
        text_ = text;
        setDirty();
    }
}

void NUILabel::onRender(NUIRenderer& renderer) {
    auto theme = getTheme();
    if (!theme) {
        return;
    }
    
    auto bounds = getBounds();
    float textSize = fontSize_ > 0.0f ? fontSize_ : theme->getFontSizeNormal();
    NUIColor textColor = getCurrentTextColor();
    
    // Draw shadow if enabled
    if (shadowEnabled_) {
        NUIColor shadowColor = NUIColor::black().withAlpha(0.5f);
        NUIPoint shadowPos = {bounds.x + 1.0f, bounds.y + 1.0f};
        renderer.drawText(text_, shadowPos, textSize, shadowColor);
    }
    
    // Draw text based on alignment
    if (textAlign_ == TextAlign::Center && verticalAlign_ == VerticalAlign::Middle) {
        renderer.drawTextCentered(text_, bounds, textSize, textColor);
    } else if (textAlign_ == TextAlign::Left && verticalAlign_ == VerticalAlign::Top) {
        NUIPoint pos = {bounds.x, bounds.y};
        renderer.drawText(text_, pos, textSize, textColor);
    } else {
        // Custom alignment - calculate position
        // This is a simplified version; full implementation would measure text
        float x = bounds.x;
        float y = bounds.y;
        
        // Note: For full alignment, we'd need text measurement
        // For now, we'll use simple positioning
        renderer.drawText(text_, NUIPoint{x, y}, textSize, textColor);
    }
    
    // Render children
    NUIComponent::onRender(renderer);
}

NUIColor NUILabel::getCurrentTextColor() const {
    auto theme = getTheme();
    
    if (useCustomColor_) {
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

} // namespace NomadUI
