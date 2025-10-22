#include "NUIDropdown.h"
#include "NUIDropdownContainer.h"
#include "NUIDropdownManager.h"
#include "NUIThemeSystem.h"
#include "../Graphics/NUIRenderer.h"
#include <algorithm>
#include <iostream>

namespace NomadUI {

// NUIDropdownItem Implementation
NUIDropdownItem::NUIDropdownItem(const std::string& text, int value)
    : text_(text), value_(value), enabled_(true) {}

void NUIDropdownItem::setText(const std::string& text) {
    text_ = text;
}

void NUIDropdownItem::setValue(int value) {
    value_ = value;
}

void NUIDropdownItem::setEnabled(bool enabled) {
    enabled_ = enabled;
}

void NUIDropdownItem::setVisible(bool visible) {
    visible_ = visible;
}

// NUIDropdown Implementation
NUIDropdown::NUIDropdown()
    : NUIComponent(),
      selectedIndex_(-1),
      placeholderText_("Select an option"),
      isOpen_(false),
      maxVisibleItems_(8),
      hoveredIndex_(-1),
      dropdownAnimProgress_(0.0f),
      hoverAnimProgress_(0.0f)
{
    // Set dropdown to render above other content
    setLayer(NUILayer::Dropdown);
    
    auto& themeManager = NUIThemeManager::getInstance();
    
    // Use theme tokens for consistent styling
    backgroundColor_ = themeManager.getColor("dropdown.background");
    borderColor_ = themeManager.getColor("dropdown.border");
    textColor_ = themeManager.getColor("dropdown.text");
    arrowColor_ = themeManager.getColor("dropdown.arrow");
    
    // Modern hover and selection colors from theme
    hoverColor_ = themeManager.getColor("dropdown.hover");
    selectedColor_ = themeManager.getColor("dropdown.selected");
    
    // Animation targets
    targetHoverIndex_ = -1;
    
    // Initialize container
    container_ = std::make_shared<NUIDropdownContainer>();
    container_->setVisible(false);
    
    // Note: Registration with manager will be done after object is fully constructed
}

void NUIDropdown::registerWithManager() {
    // Register with global manager after object is fully constructed
    NUIDropdownManager::getInstance().registerDropdown(std::static_pointer_cast<NUIDropdown>(shared_from_this()));
}

NUIDropdown::~NUIDropdown() {
    // Unregister from global manager
    NUIDropdownManager::getInstance().unregisterDropdown(std::static_pointer_cast<NUIDropdown>(shared_from_this()));
}

void NUIDropdown::onRender(NUIRenderer& renderer) {
    if (!isVisible()) return;
    
    auto& themeManager = NUIThemeManager::getInstance();
    NUIRect bounds = getBounds();
    float cornerRadius = 6.0f; // FL Studio style rounded corners
    
    // Update animations
    updateAnimations();
    
    // FL Studio-inspired color scheme
    NUIColor bgColor = themeManager.getColor("dropdown.background");
    NUIColor textColor = themeManager.getColor("dropdown.text");
    NUIColor borderColor = themeManager.getColor("dropdown.border");
    NUIColor hoverColor = themeManager.getColor("dropdown.hover");
    NUIColor focusColor = themeManager.getColor("dropdown.focus");
    
    // Apply hover state with smooth transition
    if (isHovered_) {
        bgColor = hoverColor;
    }
    
    // Apply focus state
    if (isFocused_) {
        borderColor = focusColor;
    }
    
    // Draw subtle drop shadow for depth (FL Studio style)
    NUIRect shadowBounds = bounds;
    shadowBounds.x += 1.0f;
    shadowBounds.y += 1.0f;
    renderer.fillRoundedRect(shadowBounds, cornerRadius, NUIColor(0, 0, 0, 0.15f));
    
    // Draw main background with clean borders
    renderer.fillRoundedRect(bounds, cornerRadius, bgColor);
    renderer.strokeRoundedRect(bounds, cornerRadius, 2.5f, borderColor);  // Much thicker border to match container
    
    // Render selected text or placeholder with proper padding
    std::string displayText = getSelectedText();
    if (displayText.empty()) {
        displayText = placeholderText_;
    }
    
    // FL Studio style text positioning - properly centered
    auto textSize = renderer.measureText(displayText, 14.0f);
    float textX = bounds.x + 12.0f;  // Left-aligned with padding
    // Proper vertical centering using the same formula as drawTextCentered
    float ascent = textSize.height * 0.8f;  // Approximate ascent for 14pt font
    float textY = bounds.y + bounds.height * 0.5f + ascent * 0.5f - 6.5f;
    renderer.drawText(displayText, NUIPoint(textX, textY), 14.0f, textColor);
    
    // Render modern vector-style arrow (FL Studio style)
    renderModernArrow(renderer, bounds, isOpen_);
    
    // Flush main button rendering
    renderer.flush();
    
    // Render children (like arrow, etc.)
    renderChildren(renderer);
    
    // Render container if dropdown is open
    if (isOpen_ && container_) {
        container_->onRender(renderer);
    }
}

bool NUIDropdown::onMouseEvent(const NUIMouseEvent& event) {
    if (!isVisible() || !isEnabled()) return false;
    
        // Use local bounds since we're using absolute coordinates
        NUIRect bounds = getBounds();
    
    // Handle container mouse events first if open
    if (isOpen_ && container_) {
        if (container_->onMouseEvent(event)) {
            return true;
        }
    }
    
    if (event.pressed && event.button == NUIMouseButton::Left) {
        std::cout << "===== NUIDropdown mouse click =====" << std::endl;
        std::cout << "Dropdown ID: " << getId() << std::endl;
        std::cout << "Mouse position: (" << event.position.x << ", " << event.position.y << ")" << std::endl;
        std::cout << "Dropdown global bounds: (" << bounds.x << ", " << bounds.y << ", " << bounds.width << ", " << bounds.height << ")" << std::endl;
        std::cout << "Contains click: " << bounds.contains(event.position) << std::endl;
        std::cout << "Current isOpen_: " << isOpen_ << std::endl;
        
        // Check if this is the correct dropdown for this click
        if (bounds.contains(event.position)) {
            std::cout << "*** CORRECT DROPDOWN HIT ***" << std::endl;
            auto gb = getGlobalBounds();
            std::cout << "[Dropdown] local=(" << getBounds().x << "," << getBounds().y
                      << ") global=(" << gb.x << "," << gb.y << ")" << std::endl;
            // Clicked on main dropdown button
            std::cout << "Clicked on dropdown button, toggling open state" << std::endl;
            setOpen(!isOpen_);
            return true;
        } else {
            std::cout << "*** WRONG DROPDOWN - click outside bounds ***" << std::endl;
            // Clicked outside any dropdown, close all dropdowns
            std::cout << "Clicked outside dropdown, closing all" << std::endl;
            NUIDropdownManager::getInstance().closeAllDropdowns();
                    return true;
        }
    }
    
    return NUIComponent::onMouseEvent(event);
}

bool NUIDropdown::onKeyEvent(const NUIKeyEvent& event) {
    if (!isVisible() || !isEnabled() || !isFocused()) return false;
    
    if (event.pressed) {
        if (event.keyCode == NUIKeyCode::Escape) {
            if (isOpen_) {
                closeDropdown();
                return true;
            }
        } else if (event.keyCode == NUIKeyCode::Enter) {
            if (isOpen_ && hoveredIndex_ != -1) {
                setSelectedIndex(hoveredIndex_);
                closeDropdown();
                return true;
            } else if (!isOpen_) {
                openDropdown();
                return true;
            }
        } else if (event.keyCode == NUIKeyCode::Down) {
            if (!isOpen_) {
                openDropdown();
            } else {
                hoveredIndex_ = getNextVisibleIndex(hoveredIndex_);
                setDirty(true);
            }
            return true;
        } else if (event.keyCode == NUIKeyCode::Up) {
            if (!isOpen_) {
                openDropdown();
            } else {
                hoveredIndex_ = getPreviousVisibleIndex(hoveredIndex_);
                setDirty(true);
            }
            return true;
        }
    }
    return NUIComponent::onKeyEvent(event);
}

void NUIDropdown::addItem(const std::string& text, int value) {
    items_.push_back(std::make_shared<NUIDropdownItem>(text, value));
    setDirty(true);
}

void NUIDropdown::clear() {
    items_.clear();
    selectedIndex_ = -1;
    setDirty(true);
}

void NUIDropdown::setSelectedIndex(int index) {
    if (index >= 0 && index < static_cast<int>(items_.size())) {
        selectedIndex_ = index;
        if (onSelectionChanged_) {
            onSelectionChanged_(selectedIndex_, items_[selectedIndex_]->getValue(), items_[selectedIndex_]->getText());
        }
        setDirty(true);
    }
}

std::string NUIDropdown::getSelectedText() const {
    if (selectedIndex_ >= 0 && selectedIndex_ < static_cast<int>(items_.size())) {
        return items_[selectedIndex_]->getText();
    }
    return "";
}

int NUIDropdown::getSelectedValue() const {
    if (selectedIndex_ >= 0 && selectedIndex_ < static_cast<int>(items_.size())) {
        return items_[selectedIndex_]->getValue();
    }
    return 0;
}

void NUIDropdown::setOpen(bool open) {
    std::cout << "===== NUIDropdown::setOpen() called =====" << std::endl;
    std::cout << "Requested open: " << open << ", current isOpen_: " << isOpen_ << std::endl;
    
    if (open != isOpen_) {
        if (open) {
            std::cout << "Opening dropdown..." << std::endl;
            openDropdown();
        } else {
            std::cout << "Closing dropdown..." << std::endl;
            closeDropdown();
        }
    } else {
        std::cout << "No state change needed" << std::endl;
    }
}

void NUIDropdown::setPlaceholderText(const std::string& text) {
    placeholderText_ = text;
    setDirty(true);
}

void NUIDropdown::setMaxVisibleItems(int count) {
    maxVisibleItems_ = std::max(1, count);
}

void NUIDropdown::openDropdown() {
    std::cout << "===== NUIDropdown::openDropdown() called =====" << std::endl;
    std::cout << "isOpen_: " << isOpen_ << ", items_.size(): " << items_.size() << std::endl;
    
    // Use global manager to ensure only one dropdown is open
    NUIDropdownManager::getInstance().openDropdown(std::static_pointer_cast<NUIDropdown>(shared_from_this()));
    
    isOpen_ = true;
    std::cout << "Dropdown state set to open" << std::endl;
    hoveredIndex_ = selectedIndex_; // Start hovering on selected item
    dropdownAnimProgress_ = 0.0f; // Start animation from 0
    setDirty(true);
    setFocused(true); // Grab focus when opened
    
    // Ensure container exists (persistent allocation)
    if (!container_) {
        container_ = std::make_shared<NUIDropdownContainer>();
        container_->setLayer(NUILayer::Dropdown);
        std::cout << "Created new dropdown container" << std::endl;
    }
    
    // Setup container with current state
    container_->setVisible(true);
    container_->setItems(items_);
    container_->setSelectedIndex(selectedIndex_);
    container_->setHoveredIndex(hoveredIndex_);
    container_->setMaxVisibleItems(maxVisibleItems_);
    
    // Find root component by traversing up the parent chain
    NUIComponent* root = this;
    while (root->getParent()) {
        root = root->getParent();
    }
    
    // Add container to root component for rendering (not as child of dialog)
    // This ensures it renders above all other UI elements
    if (root) {
        bool isChild = false;
        for (const auto& child : root->getChildren()) {
            if (child == container_) {
                isChild = true;
                break;
            }
        }
        if (!isChild) {
            root->addChild(container_);
            std::cout << "Added dropdown container to root component" << std::endl;
        }
    }
    
    // Set positioning AFTER adding to parent - use global coordinates
    NUIRect globalBounds = getGlobalBounds();
    container_->setSourceBounds(globalBounds);
    
    // Debug output to verify coordinate conversion
    std::cout << "[Dropdown open] " << getId() 
              << " localY=" << getBounds().y 
              << " globalY=" << globalBounds.y
              << " containerY=" << container_->getSourceBounds().y << std::endl;
    
    // Let the manager handle available space - don't override it here
    
    // Setup callbacks
    container_->setOnItemSelected([this](int index, int value, const std::string& text) {
        setSelectedIndex(index);
        closeDropdown();
    });
    
    container_->setOnClose([this]() {
        closeDropdown();
    });
    
    if (onOpen_) {
        onOpen_();
    }
}

void NUIDropdown::closeDropdown() {
    isOpen_ = false;
    hoveredIndex_ = -1;
    targetHoverIndex_ = -1;
    hoverAnimProgress_ = 0.0f; // Reset hover animation
    setDirty(true);
    setFocused(false); // Release focus when closed
    
    // Hide container (persistent allocation - just toggle visibility)
    if (container_) {
        container_->setVisible(false);
        // Remove from root component when closing
        NUIComponent* root = this;
        while (root->getParent()) {
            root = root->getParent();
        }
        if (root) {
            root->removeChild(container_);
            std::cout << "Removed dropdown container from root component" << std::endl;
        }
    }
    
    // Notify manager
    NUIDropdownManager::getInstance().closeDropdown(std::static_pointer_cast<NUIDropdown>(shared_from_this()));
    
    if (onClose_) {
        onClose_();
    }
}

bool NUIDropdown::handleItemClick(const NUIPoint& position) {
    if (!isOpen_ || items_.empty()) return false;
    
    NUIRect bounds = getBounds();
    float itemHeight = 32.0f;
    float dropdownY = bounds.y + bounds.height;
    
    // Calculate visible items count
    int visibleItemCount = 0;
    for (const auto& item : items_) {
        if (item->isVisible()) {
            visibleItemCount++;
        }
    }
    
    if (visibleItemCount > 0) {
        int visibleItems = std::min(visibleItemCount, maxVisibleItems_);
    
    // Check if click is within dropdown area
        if (position.y >= dropdownY && position.y < dropdownY + visibleItems * itemHeight) {
            int visibleIndex = static_cast<int>((position.y - dropdownY) / itemHeight);
            int clickedIndex = getVisibleItemIndex(visibleIndex);
        
        if (clickedIndex >= 0 && clickedIndex < static_cast<int>(items_.size())) {
                if (items_[clickedIndex]->isEnabled() && items_[clickedIndex]->isVisible()) {
                setSelectedIndex(clickedIndex);
                closeDropdown();
                return true;
                }
            }
        }
    }
    return false;
}

void NUIDropdown::renderItem(NUIRenderer& renderer, const NUIRect& bounds, const std::shared_ptr<NUIDropdownItem>& item, bool isHovered, bool isSelected) {
    auto& themeManager = NUIThemeManager::getInstance();
    NUIColor bgColor = backgroundColor_;
    NUIColor textColor = textColor_;
    
    // Modern translucent overlay system
    if (isHovered) {
        // Subtle hover overlay
        bgColor = hoverColor_;
    }
    
    // Selected items get highlighted text color, not background
    if (isSelected) {
        textColor = themeManager.getColor("dropdown.textSelected"); // Use theme token for selected text
    }
    
    if (!item->isEnabled()) {
        textColor = NUIColor::fromHex(0xff5A5A5D); // textDisabled
    }
    
    // Use rounded rectangles for modern look
    renderer.fillRoundedRect(bounds, 4.0f, bgColor);
    
    // Use drawTextCentered for proper vertical and horizontal centering
    renderer.drawTextCentered(item->getText(), bounds, 14.0f, textColor);
}

void NUIDropdown::updateAnimations() {
    // Update animations with delta time (assuming 60fps for now)
    float dt = 1.0f / 60.0f;
    
    // Smooth dropdown open/close animation
    openAnim_.target = isOpen_ ? 1.0f : 0.0f;
    openAnim_.update(dt);
    dropdownAnimProgress_ = openAnim_.progress;
    
    // Instantaneous hover - no animation delays
    hoverAnimProgress_ = (hoveredIndex_ >= 0) ? 1.0f : 0.0f;
    
    // Mark dirty if animations are still running
    if (openAnim_.isAnimating() || hoverAnim_.isAnimating()) {
        setDirty(true);
    }
}

void NUIDropdown::renderModernArrow(NUIRenderer& renderer, const NUIRect& bounds, bool isOpen) {
    // Draw a modern triangular arrow using lines instead of text
    float arrowSize = 6.0f;
    float arrowX = bounds.x + bounds.width - 20;
    float arrowY = bounds.y + bounds.height * 0.5f;
    
    NUIColor arrowColor = arrowColor_;
    
    if (isOpen) {
        // Upward pointing triangle
        NUIPoint top(arrowX, arrowY - arrowSize * 0.5f);
        NUIPoint left(arrowX - arrowSize * 0.5f, arrowY + arrowSize * 0.5f);
        NUIPoint right(arrowX + arrowSize * 0.5f, arrowY + arrowSize * 0.5f);
        
        renderer.drawLine(top, left, 1.5f, arrowColor);
        renderer.drawLine(left, right, 1.5f, arrowColor);
        renderer.drawLine(right, top, 1.5f, arrowColor);
    } else {
        // Downward pointing triangle
        NUIPoint bottom(arrowX, arrowY + arrowSize * 0.5f);
        NUIPoint left(arrowX - arrowSize * 0.5f, arrowY - arrowSize * 0.5f);
        NUIPoint right(arrowX + arrowSize * 0.5f, arrowY - arrowSize * 0.5f);
        
        renderer.drawLine(bottom, left, 1.5f, arrowColor);
        renderer.drawLine(left, right, 1.5f, arrowColor);
        renderer.drawLine(right, bottom, 1.5f, arrowColor);
    }
}

// Missing method implementations
void NUIDropdown::addItem(std::shared_ptr<NUIDropdownItem> item) {
    if (item) {
        items_.push_back(item);
        setDirty(true);
    }
}

void NUIDropdown::removeItem(int index) {
    if (index >= 0 && index < static_cast<int>(items_.size())) {
        items_.erase(items_.begin() + index);
        
        // Adjust selected index if necessary
        if (selectedIndex_ == index) {
            selectedIndex_ = -1;
        } else if (selectedIndex_ > index) {
            selectedIndex_--;
        }
        
        // Adjust hovered index if necessary
        if (hoveredIndex_ == index) {
            hoveredIndex_ = -1;
        } else if (hoveredIndex_ > index) {
            hoveredIndex_--;
        }
        
        setDirty(true);
    }
}

void NUIDropdown::setSelectedValue(int value) {
    for (size_t i = 0; i < items_.size(); ++i) {
        if (items_[i]->getValue() == value) {
            setSelectedIndex(static_cast<int>(i));
            return;
        }
    }
}

void NUIDropdown::setOnSelectionChanged(std::function<void(int index, int value, const std::string& text)> callback) {
    onSelectionChanged_ = callback;
}

void NUIDropdown::setOnOpen(std::function<void()> callback) {
    onOpen_ = callback;
}

void NUIDropdown::setOnClose(std::function<void()> callback) {
    onClose_ = callback;
}

void NUIDropdown::setBackgroundColor(const NUIColor& color) {
    backgroundColor_ = color;
    setDirty(true);
}

void NUIDropdown::setBorderColor(const NUIColor& color) {
    borderColor_ = color;
    setDirty(true);
}

void NUIDropdown::setTextColor(const NUIColor& color) {
    textColor_ = color;
    setDirty(true);
}

void NUIDropdown::setHoverColor(const NUIColor& color) {
    hoverColor_ = color;
    setDirty(true);
}

void NUIDropdown::setSelectedColor(const NUIColor& color) {
    selectedColor_ = color;
    setDirty(true);
}

void NUIDropdown::onMouseEnter() {
    // Could add hover effects here if needed
    NUIComponent::onMouseEnter();
}

void NUIDropdown::onMouseLeave() {
    // Reset hover state when mouse leaves
    if (hoveredIndex_ != -1) {
        hoveredIndex_ = -1;
        setDirty(true);
    }
    NUIComponent::onMouseLeave();
}

int NUIDropdown::getVisibleItemIndex(int visibleIndex) const {
    int visibleCount = 0;
    for (size_t i = 0; i < items_.size(); ++i) {
        if (items_[i]->isVisible()) {
            if (visibleCount == visibleIndex) {
                return static_cast<int>(i);
            }
            visibleCount++;
        }
    }
    return -1;
}

int NUIDropdown::getNextVisibleIndex(int currentIndex) const {
    if (items_.empty()) return -1;
    
    // Start from next index
    int startIndex = (currentIndex + 1) % static_cast<int>(items_.size());
    
    // Search for next visible item
    for (int i = 0; i < static_cast<int>(items_.size()); ++i) {
        int index = (startIndex + i) % static_cast<int>(items_.size());
        if (items_[index]->isVisible()) {
            return index;
        }
    }
    
    return -1; // No visible items found
}

int NUIDropdown::getPreviousVisibleIndex(int currentIndex) const {
    if (items_.empty()) return -1;
    
    // Start from previous index
    int startIndex = (currentIndex - 1 + static_cast<int>(items_.size())) % static_cast<int>(items_.size());
    
    // Search for previous visible item
    for (int i = 0; i < static_cast<int>(items_.size()); ++i) {
        int index = (startIndex - i + static_cast<int>(items_.size())) % static_cast<int>(items_.size());
        if (items_[index]->isVisible()) {
            return index;
        }
    }
    
    return -1; // No visible items found
}

} // namespace NomadUI
