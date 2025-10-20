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
    , hoveredButton_(HoverButton::None)
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
    NUIColor bgColor = themeManager.getColor("background"); // Use background color for flush look
    NUIColor textColor = themeManager.getColor("text");
    
    // Draw title bar background - flush with window background
    renderer.fillRect(bounds, bgColor);
    
    // No separator line for clean, flush appearance
    
    // Draw title text - left-aligned, vertically centered
    float fontSize = 13.0f; // Fixed size for consistency
    NUISize titleSize = renderer.measureText(title_, fontSize);
    // Vertically center the text in the title bar (accounting for baseline)
    float textY = bounds.y + (bounds.height - fontSize) * 0.5f + fontSize * 0.75f;
    NUIPoint titlePos(bounds.x + 12, textY); // 12px left padding for better spacing
    renderer.drawText(title_, titlePos, fontSize, textColor);
    
    // Draw window controls
    drawWindowControls(renderer);
}

void NUICustomTitleBar::drawWindowControls(NUIRenderer& renderer) {
    auto& themeManager = NUIThemeManager::getInstance();
    NUIColor textColor = themeManager.getColor("text");
    NUIColor hoverBgColor = NUIColor(1.0f, 1.0f, 1.0f, 0.1f); // Subtle white overlay
    NUIColor closeHoverBg = NUIColor(0.9f, 0.2f, 0.2f, 1.0f); // Red for close button
    
    // Modern crisp icons - smaller size, thinner lines for high-DPI displays
    float iconSize = std::min(minimizeButtonRect_.width, minimizeButtonRect_.height) * 0.35f;
    float lineThickness = 1.0f; // Thinner lines look crisper on modern displays
    
    // Draw minimize button
    if (hoveredButton_ == HoverButton::Minimize) {
        renderer.fillRect(minimizeButtonRect_, hoverBgColor);
    }
    NUIPoint minCenter(minimizeButtonRect_.x + minimizeButtonRect_.width * 0.5f,
                       minimizeButtonRect_.y + minimizeButtonRect_.height * 0.5f);
    NUIPoint minStart(minCenter.x - iconSize * 0.5f, minCenter.y);
    NUIPoint minEnd(minCenter.x + iconSize * 0.5f, minCenter.y);
    renderer.drawLine(minStart, minEnd, lineThickness, textColor);
    
    // Draw maximize/restore button
    if (hoveredButton_ == HoverButton::Maximize) {
        renderer.fillRect(maximizeButtonRect_, hoverBgColor);
    }
    NUIPoint maxCenter(maximizeButtonRect_.x + maximizeButtonRect_.width * 0.5f,
                       maximizeButtonRect_.y + maximizeButtonRect_.height * 0.5f);
    
    if (isMaximized_) {
        // Restore icon - two overlapping squares (Telegram style)
        float offset = iconSize * 0.15f;
        NUIRect backSquare(maxCenter.x - iconSize * 0.35f + offset, 
                          maxCenter.y - iconSize * 0.35f - offset, 
                          iconSize * 0.7f, iconSize * 0.7f);
        NUIRect frontSquare(maxCenter.x - iconSize * 0.35f - offset, 
                           maxCenter.y - iconSize * 0.35f + offset, 
                           iconSize * 0.7f, iconSize * 0.7f);
        renderer.strokeRect(backSquare, lineThickness, textColor);
        renderer.strokeRect(frontSquare, lineThickness, textColor);
    } else {
        // Maximize icon - single square
        NUIRect maxSquare(maxCenter.x - iconSize * 0.5f, 
                         maxCenter.y - iconSize * 0.5f, 
                         iconSize, iconSize);
        renderer.strokeRect(maxSquare, lineThickness, textColor);
    }
    
    // Draw close button with red hover
    if (hoveredButton_ == HoverButton::Close) {
        renderer.fillRect(closeButtonRect_, closeHoverBg);
    }
    NUIPoint closeCenter(closeButtonRect_.x + closeButtonRect_.width * 0.5f,
                         closeButtonRect_.y + closeButtonRect_.height * 0.5f);
    float closeSize = iconSize * 0.8f;
    NUIPoint closeTL(closeCenter.x - closeSize * 0.5f, closeCenter.y - closeSize * 0.5f);
    NUIPoint closeTR(closeCenter.x + closeSize * 0.5f, closeCenter.y - closeSize * 0.5f);
    NUIPoint closeBL(closeCenter.x - closeSize * 0.5f, closeCenter.y + closeSize * 0.5f);
    NUIPoint closeBR(closeCenter.x + closeSize * 0.5f, closeCenter.y + closeSize * 0.5f);
    NUIColor closeIconColor = (hoveredButton_ == HoverButton::Close) ? NUIColor(1.0f, 1.0f, 1.0f, 1.0f) : textColor;
    renderer.drawLine(closeTL, closeBR, lineThickness, closeIconColor);
    renderer.drawLine(closeTR, closeBL, lineThickness, closeIconColor);
}

bool NUICustomTitleBar::onMouseEvent(const NUIMouseEvent& event) {
    NUIPoint mousePos(event.position.x, event.position.y);
    
    // Update hover state
    HoverButton previousHover = hoveredButton_;
    hoveredButton_ = HoverButton::None;
    
    if (isPointInButton(mousePos, minimizeButtonRect_)) {
        hoveredButton_ = HoverButton::Minimize;
    } else if (isPointInButton(mousePos, maximizeButtonRect_)) {
        hoveredButton_ = HoverButton::Maximize;
    } else if (isPointInButton(mousePos, closeButtonRect_)) {
        hoveredButton_ = HoverButton::Close;
    }
    
    // Mark dirty if hover state changed
    if (previousHover != hoveredButton_) {
        setDirty(true);
    }
    
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
        // Window dragging is now handled by Windows via WM_NCHITTEST
        // No need to handle it here
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
