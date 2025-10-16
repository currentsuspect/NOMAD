#include "NUICustomTitleBar.h"
#include "../Graphics/NUIRenderer.h"
#include "../Core/NUIThemeSystem.h"
#include <iostream>

namespace NomadUI {

NUICustomTitleBar::NUICustomTitleBar()
    : NUIComponent()
    , title_("Nomad")
    , height_(32.0f)
    , isMaximized_(false)
    , isDragging_(false)
    , dragStartPos_(0, 0)
    , windowStartPos_(0, 0)
{
    setId("titleBar"); // Set ID for debugging
    setSize(800, height_); // Default width, will be updated by parent
    updateButtonRects();
}

void NUICustomTitleBar::setTitle(const std::string& title) {
    title_ = title;
    setDirty(true);
}

void NUICustomTitleBar::setHeight(float height) {
    height_ = height;
    setSize(getBounds().width, height);
    updateButtonRects();
    setDirty(true);
}

void NUICustomTitleBar::onRender(NUIRenderer& renderer) {
    NUIRect bounds = getBounds();
    
    
    // Get theme colors
    auto& themeManager = NUIThemeManager::getInstance();
    NUIColor bgColor = themeManager.getColor("surface");
    NUIColor textColor = themeManager.getColor("text");
    NUIColor borderColor = themeManager.getColor("border");
    
    // Draw title bar background
    renderer.fillRect(bounds, bgColor);
    
    // Draw subtle separator line with lighter color for hierarchy
    NUIColor separatorColor = themeManager.getColor("border");
    separatorColor.a *= 0.6f; // Make it more subtle
    NUIRect separatorRect(bounds.x, bounds.y + bounds.height - 1, bounds.width, 1);
    renderer.fillRect(separatorRect, separatorColor);
    
    // Draw title text - perfectly left-centered alignment
    float fontSize = std::max(12.0f, height_ * 0.4f); // Scale with title bar height
    NUISize titleSize = renderer.measureText(title_, fontSize);
    NUIPoint titlePos(bounds.x + 4, bounds.y + (bounds.height - titleSize.height) * 0.5f); // Perfect left-center alignment
    renderer.drawText(title_, titlePos, fontSize, textColor);
    
    // Draw window controls
    drawWindowControls(renderer);
}

void NUICustomTitleBar::drawWindowControls(NUIRenderer& renderer) {
    auto& themeManager = NUIThemeManager::getInstance();
    NUIColor textColor = themeManager.getColor("text");
    NUIColor hoverColor = themeManager.getColor("primary");
    
    // Responsive font size for buttons
    float buttonFontSize = std::max(10.0f, height_ * 0.35f);
    
    // Draw minimize button (-) - centered
    std::string minText = "-";
    NUISize minSize = renderer.measureText(minText, buttonFontSize);
    NUIPoint minPos(minimizeButtonRect_.x + (minimizeButtonRect_.width - minSize.width) * 0.5f,
                    minimizeButtonRect_.y + (minimizeButtonRect_.height - minSize.height) * 0.5f);
    renderer.drawText(minText, minPos, buttonFontSize, textColor);
    
    // Draw maximize/restore button (□ or ⟲) - centered
    std::string maxText = isMaximized_ ? "⟲" : "□";
    NUISize maxSize = renderer.measureText(maxText, buttonFontSize);
    NUIPoint maxPos(maximizeButtonRect_.x + (maximizeButtonRect_.width - maxSize.width) * 0.5f,
                    maximizeButtonRect_.y + (maximizeButtonRect_.height - maxSize.height) * 0.5f);
    renderer.drawText(maxText, maxPos, buttonFontSize, textColor);
    
    // Draw close button (×) - centered
    std::string closeText = "×";
    NUISize closeSize = renderer.measureText(closeText, buttonFontSize);
    NUIPoint closePos(closeButtonRect_.x + (closeButtonRect_.width - closeSize.width) * 0.5f,
                      closeButtonRect_.y + (closeButtonRect_.height - closeSize.height) * 0.5f);
    renderer.drawText(closeText, closePos, buttonFontSize, textColor);
}

bool NUICustomTitleBar::onMouseEvent(const NUIMouseEvent& event) {
    NUIPoint mousePos(event.position.x, event.position.y);
    
    if (event.pressed && event.button == NUIMouseButton::Left) {
        // Check if clicking on window controls
        if (isPointInButton(mousePos, minimizeButtonRect_)) {
            handleButtonClick(minimizeButtonRect_);
            if (onMinimize_) onMinimize_();
            return true;
        }
        else if (isPointInButton(mousePos, maximizeButtonRect_)) {
            handleButtonClick(maximizeButtonRect_);
            if (onMaximize_) onMaximize_();
            return true;
        }
        else if (isPointInButton(mousePos, closeButtonRect_)) {
            handleButtonClick(closeButtonRect_);
            if (onClose_) onClose_();
            return true;
        }
        else {
            // Start dragging
            isDragging_ = true;
            dragStartPos_ = mousePos;
            // Store current window position (this would need to be passed from window)
            windowStartPos_ = NUIPoint(0, 0); // TODO: Get actual window position
            return true;
        }
    }
    else if (!event.pressed && event.button == NUIMouseButton::Left) {
        // Stop dragging
        if (isDragging_) {
            isDragging_ = false;
            return true;
        }
    }
    else if (event.button == NUIMouseButton::None && isDragging_) {
        // Handle dragging
        if (onDrag_) {
            NUIPoint delta(mousePos.x - dragStartPos_.x, mousePos.y - dragStartPos_.y);
            onDrag_(delta.x, delta.y);
        }
        return true;
    }
    
    return NUIComponent::onMouseEvent(event);
}

void NUICustomTitleBar::onResize(int width, int height) {
    // Update our size
    setSize(width, height_);
    
    // Update button positions
    updateButtonRects();
    
    // Call parent resize
    NUIComponent::onResize(width, height);
}

void NUICustomTitleBar::updateButtonRects() {
    NUIRect bounds = getBounds();
    
    // Button dimensions
    float buttonWidth = 46.0f;
    float buttonHeight = height_ - 2.0f;
    float buttonY = bounds.y + 1.0f;
    
    // Position buttons from right edge
    float rightX = bounds.x + bounds.width;
    
    // Close button (rightmost)
    closeButtonRect_ = NUIRect(rightX - buttonWidth, buttonY, buttonWidth, buttonHeight);
    
    // Maximize button
    maximizeButtonRect_ = NUIRect(rightX - buttonWidth * 2, buttonY, buttonWidth, buttonHeight);
    
    // Minimize button
    minimizeButtonRect_ = NUIRect(rightX - buttonWidth * 3, buttonY, buttonWidth, buttonHeight);
}

bool NUICustomTitleBar::isPointInButton(const NUIPoint& point, const NUIRect& buttonRect) {
    return point.x >= buttonRect.x && point.x <= buttonRect.x + buttonRect.width &&
           point.y >= buttonRect.y && point.y <= buttonRect.y + buttonRect.height;
}

void NUICustomTitleBar::handleButtonClick(const NUIRect& buttonRect) {
    // Visual feedback for button click
    setDirty(true);
}

} // namespace NomadUI
