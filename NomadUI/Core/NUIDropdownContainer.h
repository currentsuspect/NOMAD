#pragma once

#include "NUIComponent.h"
#include "NUITypes.h"
#include <vector>
#include <memory>
#include <functional>

namespace NomadUI {

class NUIRenderer;

/**
 * A self-contained dropdown container that renders the dropdown list
 * and handles scrolling when content exceeds available space.
 * 
 * This component is created dynamically when a dropdown opens and
 * is positioned to avoid overlapping with other UI elements.
 */
class NUIDropdownContainer : public NUIComponent {
public:
    NUIDropdownContainer();
    virtual ~NUIDropdownContainer();

    // ========================================================================
    // Lifecycle
    // ========================================================================
    
    virtual void onRender(NUIRenderer& renderer) override;
    virtual bool onMouseEvent(const NUIMouseEvent& event) override;
    virtual bool onKeyEvent(const NUIKeyEvent& event) override;
    virtual void onUpdate(double deltaTime) override;

    // ========================================================================
    // Content Management
    // ========================================================================
    
    void setItems(const std::vector<std::shared_ptr<class NUIDropdownItem>>& items);
    void setSelectedIndex(int index);
    void setHoveredIndex(int index);
    void setMaxVisibleItems(int count);
    
    // ========================================================================
    // Positioning & Sizing
    // ========================================================================
    
    void setSourceBounds(const NUIRect& bounds);
    NUIRect getSourceBounds() const { return sourceBounds_; }
    void setAvailableSpace(const NUIRect& space);
    void updatePosition();
    
    // ========================================================================
    // Styling
    // ========================================================================
    
    void setBackgroundColor(const NUIColor& color);
    void setBorderColor(const NUIColor& color);
    void setTextColor(const NUIColor& color);
    void setHoverColor(const NUIColor& color);
    void setSelectedColor(const NUIColor& color);
    
    // ========================================================================
    // Callbacks
    // ========================================================================
    
    void setOnItemSelected(std::function<void(int index, int value, const std::string& text)> callback);
    void setOnClose(std::function<void()> callback);

private:
    void updateScrollPosition();
    void ensureItemVisible(int itemIndex);
    bool isItemVisible(int itemIndex) const;
    void renderItem(NUIRenderer& renderer, const NUIRect& bounds, 
                   const std::shared_ptr<class NUIDropdownItem>& item, 
                   bool isHovered, bool isSelected);
    
    // Content
    std::vector<std::shared_ptr<class NUIDropdownItem>> items_;
    int selectedIndex_ = -1;
    int hoveredIndex_ = -1;
    int maxVisibleItems_ = 8;
    
    // Positioning
    NUIRect sourceBounds_;      // The dropdown button bounds
    NUIRect availableSpace_;    // Available screen space
    float itemHeight_ = 32.0f;
    float cornerRadius_ = 6.0f;
    
    // Scrolling
    float scrollOffset_ = 0.0f;
    float maxScrollOffset_ = 0.0f;
    bool isScrollable_ = false;
    
    // Scrollbar auto-hide
    float scrollbarAlpha_ = 1.0f;
    float scrollIdleTimer_ = 0.0f;
    static constexpr float SCROLLBAR_FADE_TIME = 2.0f;
    
    // Styling
    NUIColor backgroundColor_ = NUIColor::fromHex(0xff242428);
    NUIColor borderColor_ = NUIColor::fromHex(0xff2c2c2f);
    NUIColor textColor_ = NUIColor::fromHex(0xffE5E5E8);
    NUIColor hoverColor_ = NUIColor::fromHex(0xff2e2e33);
    NUIColor selectedColor_ = NUIColor::fromHex(0xff8B7FFF);
    
    // Callbacks
    std::function<void(int index, int value, const std::string& text)> onItemSelected_;
    std::function<void()> onClose_;
    
    // Animation
    float openAnimProgress_ = 0.0f;
    float targetOpenProgress_ = 1.0f;
};

} // namespace NomadUI
