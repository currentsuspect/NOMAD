#include "NUIPanel.h"
#include "../Core/NUITheme.h"

namespace NomadUI {

NUIPanel::NUIPanel() : NUIPanel("") {
}

NUIPanel::NUIPanel(const std::string& title)
    : title_(title), titleBarEnabled_(!title.empty()) {
}

void NUIPanel::setTitle(const std::string& title) {
    if (title_ != title) {
        title_ = title;
        setDirty();
    }
}

NUIRect NUIPanel::getContentBounds() const {
    auto bounds = getBounds();
    float topOffset = titleBarEnabled_ ? titleBarHeight_ : 0.0f;
    
    return NUIRect{
        bounds.x + padding_,
        bounds.y + topOffset + padding_,
        bounds.width - padding_ * 2.0f,
        bounds.height - topOffset - padding_ * 2.0f
    };
}

void NUIPanel::onRender(NUIRenderer& renderer) {
    auto theme = getTheme();
    if (!theme) {
        return;
    }
    
    auto bounds = getBounds();
    float radius = theme->getBorderRadius();
    
    // Draw shadow if enabled
    if (shadowEnabled_) {
        renderer.drawGlow(
            bounds,
            theme->getShadowBlur(),
            0.3f,
            NUIColor::black()
        );
    }
    
    // Draw background
    renderer.fillRoundedRect(bounds, radius, getCurrentBackgroundColor());
    
    // Draw title bar if enabled
    if (titleBarEnabled_ && !title_.empty()) {
        NUIRect titleBarRect = {
            bounds.x,
            bounds.y,
            bounds.width,
            titleBarHeight_
        };
        
        // Title bar background
        renderer.fillRoundedRect(
            NUIRect{bounds.x, bounds.y, bounds.width, titleBarHeight_},
            radius,
            getCurrentTitleBarColor()
        );
        
        // Fill bottom part to square off the title bar
        renderer.fillRect(
            NUIRect{bounds.x, bounds.y + titleBarHeight_ - radius, bounds.width, radius},
            getCurrentTitleBarColor()
        );
        
        // Title text
        float titlePadding = theme->getPadding();
        NUIPoint titlePos = {
            bounds.x + titlePadding,
            bounds.y + (titleBarHeight_ - theme->getFontSizeNormal()) * 0.5f
        };
        
        renderer.drawText(
            title_,
            titlePos,
            theme->getFontSizeNormal(),
            getCurrentTitleColor()
        );
        
        // Separator line
        renderer.fillRect(
            NUIRect{bounds.x, bounds.y + titleBarHeight_, bounds.width, 1.0f},
            theme->getBorder()
        );
    }
    
    // Draw border
    if (borderEnabled_) {
        renderer.strokeRoundedRect(
            bounds,
            radius,
            theme->getBorderWidth(),
            theme->getBorder()
        );
    }
    
    // Render children
    NUIComponent::onRender(renderer);
}

NUIColor NUIPanel::getCurrentBackgroundColor() const {
    auto theme = getTheme();
    
    if (useCustomColors_) {
        return backgroundColor_;
    }
    
    if (!theme) {
        return NUIColor::fromHex(0x1a1a1a);
    }
    
    return theme->getSurface();
}

NUIColor NUIPanel::getCurrentTitleBarColor() const {
    auto theme = getTheme();
    
    if (useCustomColors_) {
        return titleBarColor_;
    }
    
    if (!theme) {
        return NUIColor::fromHex(0x2a2a2a);
    }
    
    return theme->getSurface().withBrightness(1.2f);
}

NUIColor NUIPanel::getCurrentTitleColor() const {
    auto theme = getTheme();
    
    if (useCustomColors_) {
        return titleColor_;
    }
    
    if (!theme) {
        return NUIColor::white();
    }
    
    return theme->getText();
}

} // namespace NomadUI
