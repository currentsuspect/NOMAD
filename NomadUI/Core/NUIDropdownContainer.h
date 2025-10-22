#pragma once

#include "NUIComponent.h"
#include "NUITypes.h"
#include <memory>
#include <vector>

namespace NomadUI {

class NUIDropdown;
struct NUIDropdownItem;
class NUIRenderer;

class NUIDropdownContainer : public NUIComponent {
public:
    NUIDropdownContainer();
    ~NUIDropdownContainer() override;

    void show(const std::shared_ptr<NUIDropdown>& owner,
              const std::vector<NUIDropdownItem>* items,
              int selectedIndex,
              int maxVisibleItems,
              const NUIRect& anchorBounds,
              const NUIRect& viewportBounds);
    void beginClose();
    void requestRelayout();

    void onRender(NUIRenderer& renderer) override;
    void onUpdate(double deltaTime) override;
    bool onMouseEvent(const NUIMouseEvent& event) override;
    bool onKeyEvent(const NUIKeyEvent& event) override;

    bool isAnimating() const { return animating_; }

private:
    void layoutIfNeeded(NUIRenderer& renderer);
    void updateThemeColors();
    void computeListBounds(NUIRenderer& renderer);
    void ensureSelectionVisible();
    void setHoveredIndex(int index);
    int hitTestItem(const NUIPoint& position) const;
    int nextSelectableIndex(int start, int direction) const;
    float easeOut(float t) const;
    void notifySelection(int index);
    void notifyClose();

    std::weak_ptr<NUIDropdown> owner_;
    const std::vector<NUIDropdownItem>* items_ = nullptr;

    int selectedIndex_ = -1;
    int hoveredIndex_ = -1;
    int focusIndex_ = -1;
    int maxVisibleItems_ = 8;

    NUIRect anchorBounds_;
    NUIRect viewportBounds_;
    NUIRect listBounds_;
    bool openBelow_ = true;
    bool needsLayout_ = true;

    float scrollOffset_ = 0.0f;
    float maxScrollOffset_ = 0.0f;

    float animationProgress_ = 0.0f;
    float animationTarget_ = 0.0f;
    bool animating_ = false;
    bool closing_ = false;

    // Visual configuration
    float cornerRadius_ = 8.0f;
    float itemHeight_ = 26.0f;
    float horizontalPadding_ = 12.0f;
    float verticalPadding_ = 6.0f;
    float listMargin_ = 6.0f;
    float maxListHeight_ = 240.0f;

    // Theme colors
    NUIColor listBackground_;
    NUIColor listBorder_;
    NUIColor itemBackground_;
    NUIColor itemHover_;
    NUIColor itemSelected_;
    NUIColor itemText_;
    NUIColor itemHoverText_;
    NUIColor itemDisabledText_;
    NUIColor dividerColor_;
};

} // namespace NomadUI

