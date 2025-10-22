#pragma once

#include "NUIComponent.h"
#include "NUITypes.h"
#include <functional>
#include <string>
#include <vector>
#include <memory>

namespace NomadUI {

class NUIDropdownContainer;
class NUIDropdownManager;

/**
 * NUIDropdownItem - A single item in a dropdown
 */
class NUIDropdownItem {
public:
    NUIDropdownItem(const std::string& text = "", int value = 0);
    ~NUIDropdownItem() = default;

    // Properties
    void setText(const std::string& text);
    const std::string& getText() const { return text_; }

    void setValue(int value);
    int getValue() const { return value_; }

    void setEnabled(bool enabled);
    bool isEnabled() const { return enabled_; }

    void setVisible(bool visible);
    bool isVisible() const { return visible_; }

private:
    std::string text_;
    int value_ = 0;
    bool enabled_ = true;
    bool visible_ = true;
};

/**
 * NUIDropdown - A dropdown/combobox component
 * Supports selection, hover states, and keyboard navigation
 */
class NUIDropdown : public NUIComponent {
public:
    NUIDropdown();
    ~NUIDropdown() override;

    // Component interface
    void onRender(NUIRenderer& renderer) override;
    bool onMouseEvent(const NUIMouseEvent& event) override;
    bool onKeyEvent(const NUIKeyEvent& event) override;
    void onMouseEnter() override;
    void onMouseLeave() override;

    // Dropdown management
    void addItem(const std::string& text, int value = 0);
    void addItem(std::shared_ptr<NUIDropdownItem> item);
    void clear();
    void removeItem(int index);

    // Selection
    void setSelectedIndex(int index);
    int getSelectedIndex() const { return selectedIndex_; }
    
    void setSelectedValue(int value);
    int getSelectedValue() const;
    
    std::string getSelectedText() const;

    // Dropdown state
    void setOpen(bool open);
    bool isOpen() const { return isOpen_; }

    // Visual properties
    void setPlaceholderText(const std::string& text);
    const std::string& getPlaceholderText() const { return placeholderText_; }

    void setMaxVisibleItems(int count);
    int getMaxVisibleItems() const { return maxVisibleItems_; }

    // Callbacks
    void setOnSelectionChanged(std::function<void(int index, int value, const std::string& text)> callback);
    void setOnOpen(std::function<void()> callback);
    void setOnClose(std::function<void()> callback);
    
    // Internal Access
    std::shared_ptr<NUIDropdownContainer> getContainer() const { return container_; }
    const std::vector<std::shared_ptr<NUIDropdownItem>>& getItems() const { return items_; }
    
    // Manager access
    friend class NUIDropdownManager;
    
    // Registration (call after object is fully constructed)
    void registerWithManager();

    // Styling
    void setBackgroundColor(const NUIColor& color);
    NUIColor getBackgroundColor() const { return backgroundColor_; }

    void setBorderColor(const NUIColor& color);
    NUIColor getBorderColor() const { return borderColor_; }

    void setTextColor(const NUIColor& color);
    NUIColor getTextColor() const { return textColor_; }

    void setHoverColor(const NUIColor& color);
    NUIColor getHoverColor() const { return hoverColor_; }

    void setSelectedColor(const NUIColor& color);
    NUIColor getSelectedColor() const { return selectedColor_; }

private:
    void renderItem(NUIRenderer& renderer, const NUIRect& bounds, const std::shared_ptr<NUIDropdownItem>& item, bool isHovered, bool isSelected);
    bool handleItemClick(const NUIPoint& position);
    void openDropdown();
    void closeDropdown();
    int getVisibleItemIndex(int visibleIndex) const;
    int getNextVisibleIndex(int currentIndex) const;
    int getPreviousVisibleIndex(int currentIndex) const;
    
    // Modern rendering helpers
    void updateAnimations();
    void renderModernArrow(NUIRenderer& renderer, const NUIRect& bounds, bool isOpen);

    std::vector<std::shared_ptr<NUIDropdownItem>> items_;
    int selectedIndex_ = -1;
    bool isOpen_ = false;
    int hoveredIndex_ = -1;
    int maxVisibleItems_ = 8;
    std::string placeholderText_ = "Select...";
    
    // UI state
    bool isHovered_ = false;
    bool isFocused_ = false;
    
    // Animation state
    float dropdownAnimProgress_ = 0.0f;
    float hoverAnimProgress_ = 0.0f;
    int targetHoverIndex_ = -1;
    
    // Reusable animation system
    struct NUIAnim {
        float progress = 0.0f;
        float target = 0.0f;
        float speed = 10.0f;
        
        void update(float dt) { 
            progress += (target - progress) * dt * speed; 
        }
        
        bool isAnimating() const { 
            return std::abs(progress - target) > 0.01f; 
        }
    };
    
    NUIAnim openAnim_;
    NUIAnim hoverAnim_;
    
    // Container component
    std::shared_ptr<NUIDropdownContainer> container_;

    // Callbacks
    std::function<void(int index, int value, const std::string& text)> onSelectionChanged_;
    std::function<void()> onOpen_;
    std::function<void()> onClose_;

    // Styling
    NUIColor backgroundColor_ = NUIColor::fromHex(0xff242428);  // surfaceTertiary
    NUIColor borderColor_ = NUIColor::fromHex(0xff2c2c2f);       // borderSubtle
    NUIColor textColor_ = NUIColor::fromHex(0xffE5E5E8);         // textPrimary
    NUIColor hoverColor_ = NUIColor::fromHex(0xff2e2e33);       // buttonBgHover
    NUIColor selectedColor_ = NUIColor::fromHex(0xff8B7FFF);     // primary purple
    NUIColor arrowColor_ = NUIColor::fromHex(0xffA0A0A3);        // textSecondary
};

} // namespace NomadUI