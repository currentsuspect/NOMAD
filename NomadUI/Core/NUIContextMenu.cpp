#include "NUIContextMenu.h"
#include "../Graphics/NUIRenderer.h"
#include "NUIThemeSystem.h"
#include <algorithm>

#ifdef _WIN32
#include <Windows.h>
#endif
#include <cmath>
#include <iostream>

namespace NomadUI {

// ============================================================================
// NUIContextMenuItem Implementation
// ============================================================================

NUIContextMenuItem::NUIContextMenuItem(const std::string& text, Type type)
    : text_(text)
    , type_(type)
{
}

void NUIContextMenuItem::setText(const std::string& text)
{
    text_ = text;
}

void NUIContextMenuItem::setType(Type type)
{
    type_ = type;
}

void NUIContextMenuItem::setEnabled(bool enabled)
{
    enabled_ = enabled;
}

void NUIContextMenuItem::setVisible(bool visible)
{
    visible_ = visible;
}

void NUIContextMenuItem::setChecked(bool checked)
{
    checked_ = checked;
}

void NUIContextMenuItem::setShortcut(const std::string& shortcut)
{
    shortcut_ = shortcut;
}

void NUIContextMenuItem::setIcon(const std::string& iconPath)
{
    iconPath_ = iconPath;
}

void NUIContextMenuItem::setSubmenu(std::shared_ptr<NUIContextMenu> submenu)
{
    submenu_ = submenu;
}

void NUIContextMenuItem::setOnClick(std::function<void()> callback)
{
    onClickCallback_ = callback;
}

void NUIContextMenuItem::setRadioGroup(const std::string& group)
{
    radioGroup_ = group;
}

// ============================================================================
// NUIContextMenu Implementation
// ============================================================================

NUIContextMenu::NUIContextMenu()
    : NUIComponent()
{
    setSize(200, 100); // Default size
    
    // Apply theme colors
    auto& themeManager = NUIThemeManager::getInstance();
    backgroundColor_ = themeManager.getColor("surface");
    borderColor_ = themeManager.getColor("border");
    textColor_ = themeManager.getColor("text");
    hoverColor_ = themeManager.getColor("primary"); // Use Nomad's purple for highlight
    separatorColor_ = themeManager.getColor("border");
    shortcutColor_ = themeManager.getColor("textSecondary");
}

void NUIContextMenu::onRender(NUIRenderer& renderer)
{
    if (!isVisible()) return;

    drawBackground(renderer);

    // Draw items
    for (int i = 0; i < getItemCount(); ++i)
    {
        auto item = getItem(i);
        if (item && item->isVisible())
        {
            if (item->getType() == NUIContextMenuItem::Type::Separator)
            {
                drawSeparator(renderer, i);
            }
            else
            {
                drawItem(renderer, item, i);
            }
        }
    }
}

bool NUIContextMenu::onMouseEvent(const NUIMouseEvent& event)
{
    if (!isVisible()) return false;

    NUIRect bounds = getBounds();
    if (!bounds.contains(event.position)) return false;

    int itemIndex = getItemAtPosition(event.position);

    if (event.pressed && event.button == NUIMouseButton::Left)
    {
        pressedItemIndex_ = itemIndex;
        setDirty(true);
        return true;
    }
    else if (event.released && event.button == NUIMouseButton::Left)
    {
        if (pressedItemIndex_ == itemIndex && itemIndex >= 0)
        {
            handleItemClick(itemIndex);
        }
        pressedItemIndex_ = -1;
        setDirty(true);
        return true;
    }
    else if (event.button == NUIMouseButton::None) // Mouse move
    {
        if (itemIndex != hoveredItemIndex_)
        {
            hoveredItemIndex_ = itemIndex;
            if (itemIndex >= 0)
            {
                handleItemHover(itemIndex);
            }
            setDirty(true);
        }
        return true;
    }

    return false;
}

bool NUIContextMenu::onKeyEvent(const NUIKeyEvent& event)
{
    if (!isVisible()) return false;

    if (event.pressed)
    {
        switch (event.keyCode)
        {
            case NUIKeyCode::Escape:
                hide();
                return true;
            case NUIKeyCode::Up:
                if (hoveredItemIndex_ > 0)
                {
                    hoveredItemIndex_--;
                    setDirty(true);
                }
                return true;
            case NUIKeyCode::Down:
                if (hoveredItemIndex_ < getItemCount() - 1)
                {
                    hoveredItemIndex_++;
                    setDirty(true);
                }
                return true;
            case NUIKeyCode::Enter:
            case NUIKeyCode::Space:
                if (hoveredItemIndex_ >= 0)
                {
                    handleItemClick(hoveredItemIndex_);
                }
                return true;
            default:
                break;
        }
    }

    return false;
}

void NUIContextMenu::onMouseEnter()
{
    // Context menu doesn't need mouse enter handling
}

void NUIContextMenu::onMouseLeave()
{
    hoveredItemIndex_ = -1;
    setDirty(true);
}

void NUIContextMenu::addItem(std::shared_ptr<NUIContextMenuItem> item)
{
    items_.push_back(item);
    updateLayout();
    setDirty(true);
}

void NUIContextMenu::addItem(const std::string& text, std::function<void()> callback)
{
    auto item = std::make_shared<NUIContextMenuItem>(text);
    item->setOnClick(callback);
    addItem(item);
}

void NUIContextMenu::addSeparator()
{
    auto separator = std::make_shared<NUIContextMenuItem>("", NUIContextMenuItem::Type::Separator);
    addItem(separator);
}

void NUIContextMenu::addSubmenu(const std::string& text, std::shared_ptr<NUIContextMenu> submenu)
{
    auto item = std::make_shared<NUIContextMenuItem>(text, NUIContextMenuItem::Type::Submenu);
    item->setSubmenu(submenu);
    addItem(item);
}

void NUIContextMenu::addCheckbox(const std::string& text, bool checked, std::function<void(bool)> callback)
{
    auto item = std::make_shared<NUIContextMenuItem>(text, NUIContextMenuItem::Type::Checkbox);
    item->setChecked(checked);
    if (callback)
    {
        item->setOnClick([callback, checked]() { callback(!checked); });
    }
    addItem(item);
}

void NUIContextMenu::addRadioItem(const std::string& text, const std::string& group, bool selected, std::function<void()> callback)
{
    auto item = std::make_shared<NUIContextMenuItem>(text, NUIContextMenuItem::Type::Radio);
    item->setRadioGroup(group);
    item->setChecked(selected);
    item->setOnClick(callback);
    addItem(item);
}

void NUIContextMenu::clear()
{
    items_.clear();
    hoveredItemIndex_ = -1;
    pressedItemIndex_ = -1;
    updateLayout();
    setDirty(true);
}

void NUIContextMenu::showAt(const NUIPoint& position)
{
    showAt(static_cast<int>(position.x), static_cast<int>(position.y));
}

void NUIContextMenu::showAt(int x, int y)
{
    updateLayout();
    float menuWidth = getBounds().width;
    float menuHeight = getBounds().height;
    
    // Smart positioning - near cursor but adjust if off-screen
    float posX = x;
    float posY = y;
    
    // Get parent window bounds (the actual window the menu should stay within)
    NUIRect parentBounds = getParent() ? getParent()->getBounds() : NUIRect(0, 0, 800, 600);
    float windowRight = parentBounds.x + parentBounds.width;
    float windowBottom = parentBounds.y + parentBounds.height;
    
    // Check if menu would go off the right edge of the window - move it left
    if (posX + menuWidth > windowRight) {
        posX = windowRight - menuWidth - 10; // 10px margin from window edge
    }
    
    // Check if menu would go off the bottom edge of the window - move it up
    if (posY + menuHeight > windowBottom) {
        posY = windowBottom - menuHeight - 10; // 10px margin from window edge
    }
    
    // Ensure menu doesn't go off the left or top edges of the window
    if (posX < parentBounds.x) posX = parentBounds.x + 10; // 10px margin from window edge
    if (posY < parentBounds.y) posY = parentBounds.y + 10; // 10px margin from window edge
    
    setPosition(posX, posY);
    isVisible_ = true;
    hoveredItemIndex_ = 0; // Start with first item selected
    pressedItemIndex_ = -1;
    triggerShow();
    setDirty(true);
}

void NUIContextMenu::hide()
{
    isVisible_ = false;
    hoveredItemIndex_ = -1;
    pressedItemIndex_ = -1;
    hideSubmenu();
    triggerHide();
    setDirty(true);
}

void NUIContextMenu::setBackgroundColor(const NUIColor& color)
{
    backgroundColor_ = color;
    setDirty(true);
}

void NUIContextMenu::setBorderColor(const NUIColor& color)
{
    borderColor_ = color;
    setDirty(true);
}

void NUIContextMenu::setTextColor(const NUIColor& color)
{
    textColor_ = color;
    setDirty(true);
}

void NUIContextMenu::setHoverColor(const NUIColor& color)
{
    hoverColor_ = color;
    setDirty(true);
}

void NUIContextMenu::setSeparatorColor(const NUIColor& color)
{
    separatorColor_ = color;
    setDirty(true);
}

void NUIContextMenu::setShortcutColor(const NUIColor& color)
{
    shortcutColor_ = color;
    setDirty(true);
}

void NUIContextMenu::setBorderWidth(float width)
{
    borderWidth_ = width;
    setDirty(true);
}

void NUIContextMenu::setBorderRadius(float radius)
{
    borderRadius_ = radius;
    setDirty(true);
}

void NUIContextMenu::setItemHeight(float height)
{
    itemHeight_ = height;
    updateLayout();
    setDirty(true);
}

void NUIContextMenu::setItemPadding(float padding)
{
    itemPadding_ = padding;
    updateLayout();
    setDirty(true);
}

void NUIContextMenu::setIconSize(float size)
{
    iconSize_ = size;
    setDirty(true);
}

void NUIContextMenu::setAutoHide(bool autoHide)
{
    autoHide_ = autoHide;
}

void NUIContextMenu::setCloseOnSelection(bool close)
{
    closeOnSelection_ = close;
}

void NUIContextMenu::setMaxHeight(float height)
{
    maxHeight_ = height;
    updateLayout();
    setDirty(true);
}

void NUIContextMenu::setScrollable(bool scrollable)
{
    scrollable_ = scrollable;
    updateLayout();
    setDirty(true);
}

void NUIContextMenu::setOnShow(std::function<void()> callback)
{
    onShowCallback_ = callback;
}

void NUIContextMenu::setOnHide(std::function<void()> callback)
{
    onHideCallback_ = callback;
}

void NUIContextMenu::setOnItemClick(std::function<void(std::shared_ptr<NUIContextMenuItem>)> callback)
{
    onItemClickCallback_ = callback;
}

void NUIContextMenu::navigateUp()
{
    if (hoveredItemIndex_ > 0) {
        hoveredItemIndex_--;
        setDirty(true);
    }
}

void NUIContextMenu::navigateDown()
{
    if (hoveredItemIndex_ < getItemCount() - 1) {
        hoveredItemIndex_++;
        setDirty(true);
    }
}


std::shared_ptr<NUIContextMenuItem> NUIContextMenu::getItem(int index) const
{
    if (index >= 0 && index < static_cast<int>(items_.size()))
    {
        return items_[index];
    }
    return nullptr;
}

void NUIContextMenu::drawBackground(NUIRenderer& renderer)
{
    NUIRect bounds = getBounds();
    
    // Draw background
    renderer.fillRoundedRect(bounds, borderRadius_, backgroundColor_);
    
    // Draw border
    renderer.strokeRoundedRect(bounds, borderRadius_, borderWidth_, borderColor_);
}

void NUIContextMenu::drawItem(NUIRenderer& renderer, std::shared_ptr<NUIContextMenuItem> item, int index)
{
    if (!item || !item->isVisible()) return;

    NUIRect itemRect = getItemRect(index);
    
    // Draw hover background with smooth effect
    if (index == hoveredItemIndex_)
    {
        // Use Nomad's purple for highlight - proper purple color
        NUIColor purpleHighlight = NUIColor(0.6f, 0.3f, 0.9f, 0.8f); // Real purple color
        renderer.fillRoundedRect(itemRect, borderRadius_, purpleHighlight);
    }
    
    // Draw item content
    float x = itemRect.x + itemPadding_;
    float y = itemRect.y + itemRect.height * 0.5f;
    
    // Draw icon if present
    if (!item->getIcon().empty())
    {
        // TODO: Draw icon when icon system is implemented
        x += iconSize_ + itemPadding_;
    }
    
    // Draw checkbox/radio indicator
    if (item->getType() == NUIContextMenuItem::Type::Checkbox || 
        item->getType() == NUIContextMenuItem::Type::Radio)
    {
        float indicatorSize = 12.0f;
        NUIRect indicatorRect(x, y - indicatorSize * 0.5f, indicatorSize, indicatorSize);
        
        // Draw indicator background
        renderer.strokeRoundedRect(indicatorRect, 2.0f, 1.0f, borderColor_);
        
        // Draw checkmark or dot
        if (item->isChecked())
        {
            if (item->getType() == NUIContextMenuItem::Type::Checkbox)
            {
                // Draw checkmark
                NUIPoint center = indicatorRect.center();
                float size = indicatorSize * 0.6f;
                NUIPoint p1(center.x - size * 0.3f, center.y);
                NUIPoint p2(center.x - size * 0.1f, center.y + size * 0.2f);
                NUIPoint p3(center.x + size * 0.3f, center.y - size * 0.2f);
                renderer.drawLine(p1, p2, 2.0f, textColor_);
                renderer.drawLine(p2, p3, 2.0f, textColor_);
            }
            else
            {
                // Draw radio dot
                renderer.fillCircle(indicatorRect.center(), indicatorSize * 0.3f, textColor_);
            }
        }
        
        x += indicatorSize + itemPadding_;
    }
    
    // Draw text
    NUIColor textColor = item->isEnabled() ? textColor_ : textColor_.withAlpha(0.5f);
    
    // Draw default icons for common actions (using ASCII for compatibility)
    std::string icon = "";
    if (item->getText() == "Cut") icon = "X";
    else if (item->getText() == "Copy") icon = "C";
    else if (item->getText() == "Paste") icon = "V";
    else if (item->getText() == "Settings") icon = "S";
    
    if (!icon.empty())
    {
        // Draw icon with slightly different color for visibility
        NUIColor iconColor = textColor;
        iconColor.r = 0.7f; // Slightly dimmer
        renderer.drawText(icon, NUIPoint(x, y), 12.0f, iconColor);
        x += 12.0f; // Minimal space for icon
    }
    renderer.drawText(item->getText(), NUIPoint(x, y), 14.0f, textColor); // Larger text
    
    // Draw shortcut
    if (!item->getShortcut().empty())
    {
        // Position shortcut with proper spacing from the right edge
        float shortcutX = itemRect.x + itemRect.width - itemPadding_ - 40.0f; // Much more space from edge
        renderer.drawText(item->getShortcut(), NUIPoint(shortcutX, y), 12.0f, shortcutColor_);
    }
    
    // Draw submenu arrow
    if (item->getType() == NUIContextMenuItem::Type::Submenu)
    {
        drawSubmenuArrow(renderer, index);
    }
}

void NUIContextMenu::drawSeparator(NUIRenderer& renderer, int index)
{
    NUIRect itemRect = getItemRect(index);
    float centerY = itemRect.y + itemRect.height * 0.5f;
    
    NUIPoint p1(itemRect.x + itemPadding_, centerY);
    NUIPoint p2(itemRect.x + itemRect.width - itemPadding_, centerY);
    
    renderer.drawLine(p1, p2, 1.0f, separatorColor_);
}

void NUIContextMenu::drawSubmenuArrow(NUIRenderer& renderer, int index)
{
    NUIRect itemRect = getItemRect(index);
    float arrowX = itemRect.x + itemRect.width - itemPadding_ - 8.0f;
    float arrowY = itemRect.y + itemRect.height * 0.5f;
    
    // Draw right-pointing arrow
    NUIPoint p1(arrowX, arrowY - 3.0f);
    NUIPoint p2(arrowX + 6.0f, arrowY);
    NUIPoint p3(arrowX, arrowY + 3.0f);
    
    renderer.drawLine(p1, p2, 1.0f, textColor_);
    renderer.drawLine(p2, p3, 1.0f, textColor_);
}

void NUIContextMenu::updateLayout()
{
    updateSize();
}

NUIRect NUIContextMenu::getItemRect(int index) const
{
    if (index < 0 || index >= getItemCount()) return NUIRect();
    
    NUIRect bounds = getBounds();
    float y = bounds.y + index * itemHeight_;
    
    return NUIRect(bounds.x, y, bounds.width, itemHeight_);
}

float NUIContextMenu::calculateMenuHeight() const
{
    int visibleItems = 0;
    for (const auto& item : items_)
    {
        if (item && item->isVisible())
        {
            visibleItems++;
        }
    }
    
    float height = visibleItems * itemHeight_;
    return (height < maxHeight_) ? height : maxHeight_;
}

int NUIContextMenu::getItemAtPosition(const NUIPoint& position) const
{
    NUIRect bounds = getBounds();
    if (!bounds.contains(position)) return -1;
    
    float relativeY = position.y - bounds.y;
    int index = static_cast<int>(relativeY / itemHeight_);
    
    if (index >= 0 && index < getItemCount())
    {
        auto item = getItem(index);
        if (item && item->isVisible())
        {
            return index;
        }
    }
    
    return -1;
}

void NUIContextMenu::handleItemClick(int index)
{
    auto item = getItem(index);
    if (!item || !item->isEnabled()) return;
    
    // Handle radio group selection
    if (item->getType() == NUIContextMenuItem::Type::Radio && !item->getRadioGroup().empty())
    {
        // Uncheck other items in the same group
        for (auto& otherItem : items_)
        {
            if (otherItem && otherItem != item && 
                otherItem->getRadioGroup() == item->getRadioGroup())
            {
                otherItem->setChecked(false);
            }
        }
        item->setChecked(true);
    }
    
    // Handle submenu
    if (item->getType() == NUIContextMenuItem::Type::Submenu && item->getSubmenu() != nullptr)
    {
        showSubmenu(index);
        return;
    }
    
    // Trigger item click
    triggerItemClick(item);
    
    // Close menu if configured to do so
    if (closeOnSelection_)
    {
        hide();
    }
}

void NUIContextMenu::handleItemHover(int index)
{
    auto item = getItem(index);
    if (item && item->getType() == NUIContextMenuItem::Type::Submenu && item->getSubmenu() != nullptr)
    {
        showSubmenu(index);
    }
    else if (activeSubmenu_)
    {
        hideSubmenu();
    }
}

void NUIContextMenu::updateSize()
{
    float height = calculateMenuHeight();
    float width = 280.0f; // Much wider for proper shortcut spacing
    
    setSize(width, height);
}

void NUIContextMenu::showSubmenu(int itemIndex)
{
    auto item = getItem(itemIndex);
    if (!item || item->getSubmenu() == nullptr) return;
    
    hideSubmenu();
    
    activeSubmenu_ = item->getSubmenu();
    submenuItemIndex_ = itemIndex;
    
    // Position submenu to the right of the current menu
    NUIRect itemRect = getItemRect(itemIndex);
    NUIPoint submenuPos(itemRect.x + itemRect.width, itemRect.y);
    activeSubmenu_->showAt(submenuPos);
}

void NUIContextMenu::hideSubmenu()
{
    if (activeSubmenu_)
    {
        activeSubmenu_->hide();
        activeSubmenu_ = nullptr;
        submenuItemIndex_ = -1;
    }
}

void NUIContextMenu::triggerItemClick(std::shared_ptr<NUIContextMenuItem> item)
{
    if (item->getOnClick())
    {
        item->getOnClick()();
    }
    
    if (onItemClickCallback_)
    {
        onItemClickCallback_(item);
    }
}

void NUIContextMenu::triggerShow()
{
    if (onShowCallback_)
    {
        onShowCallback_();
    }
}

void NUIContextMenu::triggerHide()
{
    if (onHideCallback_)
    {
        onHideCallback_();
    }
}

} // namespace NomadUI
