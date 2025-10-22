#include "NUIDropdownContainer.h"
#include "NUIDropdown.h"
#include "NUIThemeSystem.h"
#include "../Graphics/NUIRenderer.h"
#include <algorithm>
#include <cmath>
#include <iostream>

namespace NomadUI {

NUIDropdownContainer::NUIDropdownContainer() {
    // Set to highest layer to ensure it renders above everything
    setLayer(NUILayer::Dropdown);
    
    auto& themeManager = NUIThemeManager::getInstance();
    backgroundColor_ = themeManager.getColor("surface").withAlpha(0.95f);
    borderColor_ = themeManager.getColor("borderSubtle").withAlpha(0.6f);
    textColor_ = themeManager.getColor("textPrimary");
    hoverColor_ = NUIColor(120, 90, 255, 0.15f);
    selectedColor_ = themeManager.getColor("primary");
}

NUIDropdownContainer::~NUIDropdownContainer() {
}

void NUIDropdownContainer::onRender(NUIRenderer& renderer) {
    if (!isVisible()) {
        std::cout << "Dropdown container not visible" << std::endl;
        return;
    }
    
    std::cout << "Rendering dropdown container" << std::endl;
    
    updateScrollPosition();
    
    // Update scrollbar auto-hide (assuming 60fps)
    float dt = 1.0f / 60.0f;
    scrollIdleTimer_ += dt;
    
    if (scrollIdleTimer_ > SCROLLBAR_FADE_TIME) {
        // Fade out scrollbar
        scrollbarAlpha_ += (0.0f - scrollbarAlpha_) * dt * 5.0f;
    } else {
        // Keep scrollbar visible
        scrollbarAlpha_ += (1.0f - scrollbarAlpha_) * dt * 5.0f;
    }
    
    NUIRect bounds = getBounds();
    
    // FL Studio-style drop shadow for depth
    NUIColor shadowColor = NUIColor(0, 0, 0, 0.25f);
    renderer.drawShadow(bounds, 0, 2, 8, shadowColor);
    
    // FL Studio-inspired container background
    auto& themeManager = NUIThemeManager::getInstance();
    NUIColor bgColor = themeManager.getColor("dropdown.list.background");
    NUIColor borderColor = themeManager.getColor("dropdown.list.border");
    
    // Draw container background with clean borders
    renderer.fillRoundedRect(bounds, cornerRadius_, bgColor);
    renderer.strokeRoundedRect(bounds, cornerRadius_, 2.5f, borderColor); 
    
    // Set up clipping for scrollable content
    renderer.setClipRect(bounds);
    
    // Render items with scroll offset
    int visibleItemCount = 0;
    for (const auto& item : items_) {
        if (item->isVisible()) {
            visibleItemCount++;
        }
    }
    
    if (visibleItemCount > 0) {
        int visibleItems = std::min(visibleItemCount, maxVisibleItems_);
        
        for (int i = 0; i < visibleItems; ++i) {
            int actualIndex = -1;
            int visibleIndex = 0;
            
            // Find the actual item index for this visible position
            for (size_t j = 0; j < items_.size(); ++j) {
                if (items_[j]->isVisible()) {
                    if (visibleIndex == i) {
                        actualIndex = static_cast<int>(j);
                        break;
                    }
                    visibleIndex++;
                }
            }
            
            if (actualIndex >= 0 && actualIndex < static_cast<int>(items_.size())) {
                float itemY = bounds.y + i * itemHeight_ - scrollOffset_;
                NUIRect itemBounds(bounds.x, itemY, bounds.width, itemHeight_);
                
                bool isHovered = (actualIndex == hoveredIndex_);
                bool isSelected = (actualIndex == selectedIndex_);
                
                renderItem(renderer, itemBounds, items_[actualIndex], isHovered, isSelected);
            }
        }
    }
    
    // Clear clipping
    renderer.clearClipRect();
    
    // Render scrollbar if needed (with auto-hide)
    if (isScrollable_ && maxScrollOffset_ > 0 && scrollbarAlpha_ > 0.01f) {
        float scrollbarWidth = 8.0f;
        float scrollbarX = bounds.x + bounds.width - scrollbarWidth - 4;
        float scrollbarY = bounds.y + 4;
        float scrollbarHeight = bounds.height - 8;
        
        // Scrollbar track with alpha
        NUIRect trackBounds(scrollbarX, scrollbarY, scrollbarWidth, scrollbarHeight);
        NUIColor trackColor = NUIColor(0, 0, 0, 0.3f * scrollbarAlpha_);
        renderer.fillRoundedRect(trackBounds, 4.0f, trackColor);
        
        // Calculate thumb size and position
        float totalContentHeight = visibleItemCount * itemHeight_;
        float thumbHeight = std::max(20.0f, scrollbarHeight * (bounds.height / totalContentHeight));
        float thumbY = scrollbarY + (scrollOffset_ / maxScrollOffset_) * (scrollbarHeight - thumbHeight);
        
        NUIRect thumbBounds(scrollbarX, thumbY, scrollbarWidth, thumbHeight);
        NUIColor thumbColor = NUIColor(255, 255, 255, 0.8f * scrollbarAlpha_);
        renderer.fillRoundedRect(thumbBounds, 4.0f, thumbColor);
    }
    
    renderer.flush();
}

bool NUIDropdownContainer::onMouseEvent(const NUIMouseEvent& event) {
    if (!isVisible() || !isEnabled()) return false;
    
    NUIRect bounds = getBounds();
    
    // Handle mouse wheel scrolling
        if (event.wheelDelta != 0 && isScrollable_) {
            float scrollSpeed = 32.0f; // pixels per wheel step
            scrollOffset_ -= event.wheelDelta * scrollSpeed;
            scrollOffset_ = std::max(0.0f, std::min(scrollOffset_, maxScrollOffset_));
            
            // Reset scrollbar idle timer
            scrollIdleTimer_ = 0.0f;
            scrollbarAlpha_ = 1.0f;
            
            setDirty(true);
            return true;
        }
    
    if (event.pressed && event.button == NUIMouseButton::Left) {
        if (bounds.contains(event.position)) {
            // Calculate which item was clicked
            float relativeY = event.position.y - bounds.y + scrollOffset_;
            int itemIndex = static_cast<int>(relativeY / itemHeight_);
            
            // Find the actual item index
            int actualIndex = -1;
            int visibleIndex = 0;
            for (size_t i = 0; i < items_.size(); ++i) {
                if (items_[i]->isVisible()) {
                    if (visibleIndex == itemIndex) {
                        actualIndex = static_cast<int>(i);
                        break;
                    }
                    visibleIndex++;
                }
            }
            
            if (actualIndex >= 0 && actualIndex < static_cast<int>(items_.size())) {
                selectedIndex_ = actualIndex;
                if (onItemSelected_) {
                    auto& item = items_[actualIndex];
                    onItemSelected_(actualIndex, item->getValue(), item->getText());
                }
                return true;
            }
        } else {
            // Clicked outside, close dropdown
            if (onClose_) {
                onClose_();
            }
            return true;
        }
    }
    
    // Handle hover
    if (bounds.contains(event.position)) {
        float relativeY = event.position.y - bounds.y + scrollOffset_;
        int itemIndex = static_cast<int>(relativeY / itemHeight_);
        
        // Find the actual item index
        int actualIndex = -1;
        int visibleIndex = 0;
        for (size_t i = 0; i < items_.size(); ++i) {
            if (items_[i]->isVisible()) {
                if (visibleIndex == itemIndex) {
                    actualIndex = static_cast<int>(i);
                    break;
                }
                visibleIndex++;
            }
        }
        
        hoveredIndex_ = actualIndex;
        setDirty(true);
    } else {
        hoveredIndex_ = -1;
        setDirty(true);
    }
    
    return false;
}

bool NUIDropdownContainer::onKeyEvent(const NUIKeyEvent& event) {
    if (!isVisible() || !isEnabled()) return false;
    
    if (event.pressed) {
        if (event.keyCode == NUIKeyCode::Escape) {
            if (onClose_) {
                onClose_();
            }
            return true;
        }
        
        // Handle arrow key navigation
        if (event.keyCode == NUIKeyCode::Up || event.keyCode == NUIKeyCode::Down) {
            int direction = (event.keyCode == NUIKeyCode::Down) ? 1 : -1;
            int newIndex = hoveredIndex_ + direction;
            
            // Find next visible item
            int visibleCount = 0;
            for (const auto& item : items_) {
                if (item->isVisible()) visibleCount++;
            }
            
            if (visibleCount > 0) {
                newIndex = std::max(0, std::min(newIndex, visibleCount - 1));
                
                // Convert to actual index
                int actualIndex = -1;
                int visibleIndex = 0;
                for (size_t i = 0; i < items_.size(); ++i) {
                    if (items_[i]->isVisible()) {
                        if (visibleIndex == newIndex) {
                            actualIndex = static_cast<int>(i);
                            break;
                        }
                        visibleIndex++;
                    }
                }
                
                hoveredIndex_ = actualIndex;
                ensureItemVisible(actualIndex);
                setDirty(true);
            }
            return true;
        }
        
        if (event.keyCode == NUIKeyCode::Enter) {
            if (hoveredIndex_ >= 0 && hoveredIndex_ < static_cast<int>(items_.size())) {
                selectedIndex_ = hoveredIndex_;
                if (onItemSelected_) {
                    auto& item = items_[hoveredIndex_];
                    onItemSelected_(hoveredIndex_, item->getValue(), item->getText());
                }
            }
            return true;
        }
    }
    
    return false;
}

void NUIDropdownContainer::onUpdate(double deltaTime) {
    // Smooth open animation
    float diff = targetOpenProgress_ - openAnimProgress_;
    if (std::abs(diff) > 0.01f) {
        openAnimProgress_ += diff * 0.15f;
        setDirty(true);
    }
}

void NUIDropdownContainer::setItems(const std::vector<std::shared_ptr<NUIDropdownItem>>& items) {
    items_ = items;
    updateScrollPosition();
    setDirty(true);
}

void NUIDropdownContainer::setSelectedIndex(int index) {
    selectedIndex_ = index;
    setDirty(true);
}

void NUIDropdownContainer::setHoveredIndex(int index) {
    hoveredIndex_ = index;
    setDirty(true);
}

void NUIDropdownContainer::setMaxVisibleItems(int count) {
    maxVisibleItems_ = std::max(1, count);
    updateScrollPosition();
    setDirty(true);
}

void NUIDropdownContainer::setSourceBounds(const NUIRect& bounds) {
    sourceBounds_ = bounds;
    updatePosition();
}

void NUIDropdownContainer::setAvailableSpace(const NUIRect& space) {
    availableSpace_ = space;
    updatePosition();
}

void NUIDropdownContainer::updatePosition() {
    if (items_.empty()) return;
    
    // Count visible items
    int visibleItemCount = 0;
    for (const auto& item : items_) {
        if (item->isVisible()) {
            visibleItemCount++;
        }
    }
    
    if (visibleItemCount == 0) return;
    
    int visibleItems = std::min(visibleItemCount, maxVisibleItems_);
    float dropdownHeight = visibleItems * itemHeight_;
    
    // Position as extension of source button - no gap, thicker border
    float margin = 0.0f; // No gap - attach directly
    float dropdownX = sourceBounds_.x;
    float dropdownY = sourceBounds_.y + sourceBounds_.height + margin;
    float dropdownWidth = sourceBounds_.width;
    
    // Ensure reasonable minimum size
    dropdownWidth = std::max(dropdownWidth, 120.0f);
    dropdownHeight = std::max(dropdownHeight, itemHeight_);
    
    // Basic bounds checking - only prevent going off-screen
    if (dropdownX < availableSpace_.x) {
        dropdownX = availableSpace_.x;
    }
    if (dropdownY < availableSpace_.y) {
        dropdownY = availableSpace_.y;
    }
    if (dropdownX + dropdownWidth > availableSpace_.x + availableSpace_.width) {
        dropdownX = availableSpace_.x + availableSpace_.width - dropdownWidth;
    }
    if (dropdownY + dropdownHeight > availableSpace_.y + availableSpace_.height) {
        dropdownY = availableSpace_.y + availableSpace_.height - dropdownHeight;
    }
    
    setBounds(dropdownX, dropdownY, dropdownWidth, dropdownHeight);
    updateScrollPosition();
    
    // Debug output for positioning
    std::cout << "Dropdown positioned: (" << dropdownX << ", " << dropdownY << ") "
              << dropdownWidth << "x" << dropdownHeight
              << " (source: " << sourceBounds_.x << ", " << sourceBounds_.y << " "
              << sourceBounds_.width << "x" << sourceBounds_.height << ")"
              << " (available: " << availableSpace_.x << ", " << availableSpace_.y << " "
              << availableSpace_.width << "x" << availableSpace_.height << ")"
              << " (visible: " << (isVisible() ? "true" : "false") << ")" << std::endl;
}

void NUIDropdownContainer::updateScrollPosition() {
    int visibleItemCount = 0;
    for (const auto& item : items_) {
        if (item->isVisible()) {
            visibleItemCount++;
        }
    }
    
    if (visibleItemCount <= maxVisibleItems_) {
        isScrollable_ = false;
        scrollOffset_ = 0.0f;
        maxScrollOffset_ = 0.0f;
    } else {
        isScrollable_ = true;
        float totalHeight = visibleItemCount * itemHeight_;
        float visibleHeight = maxVisibleItems_ * itemHeight_;
        maxScrollOffset_ = totalHeight - visibleHeight;
        scrollOffset_ = std::max(0.0f, std::min(scrollOffset_, maxScrollOffset_));
    }
}

void NUIDropdownContainer::ensureItemVisible(int itemIndex) {
    if (!isScrollable_) return;
    
    // Find the visible position of this item
    int visiblePosition = 0;
    for (int i = 0; i < itemIndex; ++i) {
        if (i < static_cast<int>(items_.size()) && items_[i]->isVisible()) {
            visiblePosition++;
        }
    }
    
    float itemY = visiblePosition * itemHeight_;
    float itemBottom = itemY + itemHeight_;
    
    if (itemY < scrollOffset_) {
        scrollOffset_ = itemY;
    } else if (itemBottom > scrollOffset_ + maxVisibleItems_ * itemHeight_) {
        scrollOffset_ = itemBottom - maxVisibleItems_ * itemHeight_;
    }
    
    scrollOffset_ = std::max(0.0f, std::min(scrollOffset_, maxScrollOffset_));
    setDirty(true);
}

bool NUIDropdownContainer::isItemVisible(int itemIndex) const {
    if (!isScrollable_) return true;
    
    int visiblePosition = 0;
    for (int i = 0; i < itemIndex; ++i) {
        if (i < static_cast<int>(items_.size()) && items_[i]->isVisible()) {
            visiblePosition++;
        }
    }
    
    float itemY = visiblePosition * itemHeight_;
    float itemBottom = itemY + itemHeight_;
    
    return itemY >= scrollOffset_ && itemBottom <= scrollOffset_ + maxVisibleItems_ * itemHeight_;
}

void NUIDropdownContainer::renderItem(NUIRenderer& renderer, const NUIRect& bounds, 
                                     const std::shared_ptr<NUIDropdownItem>& item, 
                                     bool isHovered, bool isSelected) {
    auto& themeManager = NUIThemeManager::getInstance();
    
    // Purple text as default, purple strip on hover
    NUIColor bgColor = themeManager.getColor("dropdown.item.background");
    NUIColor textColor = themeManager.getColor("dropdown.item.text");
    NUIColor hoverColor = themeManager.getColor("dropdown.item.hover");
    NUIColor hoverTextColor = themeManager.getColor("dropdown.item.hoverText");
    NUIColor selectedTextColor = themeManager.getColor("dropdown.item.selectedText");
    
    // Apply hover background (purple strip) and text highlighting
    if (isHovered) {
        bgColor = hoverColor;  // Purple strip background
        textColor = hoverTextColor;  // Keep purple text
    }
    
    // Apply unique text highlighting for selected item
    if (isSelected) {
        textColor = selectedTextColor;  // Purple text for selected
    }
    
    // Disabled state
    if (!item->isEnabled()) {
        textColor = themeManager.getColor("dropdown.item.disabled");
    }
    
    // Draw item background with smooth rounded corners
    renderer.fillRoundedRect(bounds, 4.0f, bgColor);
    
    // Draw divider line between items (except for the last item)
    if (!isSelected) { // Only draw divider if not the selected item
        auto& themeManager = NUIThemeManager::getInstance();
        NUIColor dividerColor = themeManager.getColor("dropdown.item.divider");
        renderer.drawLine(NUIPoint(bounds.x, bounds.y + bounds.height - 1), 
                         NUIPoint(bounds.x + bounds.width, bounds.y + bounds.height - 1), 
                         1.0f, dividerColor);
    }
    
    // Left-aligned text, vertically centered in dropdown items
    auto textSize = renderer.measureText(item->getText(), 14.0f);
    float textX = bounds.x + 12.0f;  // Left-aligned with padding
    float textY = bounds.y + bounds.height * 0.5f + textSize.height * 0.5f - 6.5f;  // Properly vertically centered
    renderer.drawText(item->getText(), NUIPoint(textX, textY), 14.0f, textColor);
}

void NUIDropdownContainer::setBackgroundColor(const NUIColor& color) {
    backgroundColor_ = color;
    setDirty(true);
}

void NUIDropdownContainer::setBorderColor(const NUIColor& color) {
    borderColor_ = color;
    setDirty(true);
}

void NUIDropdownContainer::setTextColor(const NUIColor& color) {
    textColor_ = color;
    setDirty(true);
}

void NUIDropdownContainer::setHoverColor(const NUIColor& color) {
    hoverColor_ = color;
    setDirty(true);
}

void NUIDropdownContainer::setSelectedColor(const NUIColor& color) {
    selectedColor_ = color;
    setDirty(true);
}

void NUIDropdownContainer::setOnItemSelected(std::function<void(int index, int value, const std::string& text)> callback) {
    onItemSelected_ = callback;
}

void NUIDropdownContainer::setOnClose(std::function<void()> callback) {
    onClose_ = callback;
}

} // namespace NomadUI
