#include "NUIDropdownContainer.h"
#include "NUIDropdown.h"
#include "NUIThemeSystem.h"
#include "../Graphics/NUIRenderer.h"

#include <algorithm>
#include <cmath>

namespace NomadUI {
namespace {
constexpr float kAnimationSpeed = 12.0f;
constexpr float kScrollSpeed = 32.0f;
constexpr float kShadowOpacity = 0.28f;
constexpr float kShadowBlur = 12.0f;
constexpr float kShadowOffsetY = 4.0f;
constexpr float kScrollBarWidth = 6.0f;
constexpr float kScrollBarRadius = 3.0f;
constexpr float kItemCornerRadius = 4.0f;
constexpr float kListFontSize = 14.0f;

float clamp01(float value) {
    return std::max(0.0f, std::min(1.0f, value));
}

float approach(float current, float target, float delta) {
    if (current < target) {
        current = std::min(target, current + delta);
    } else {
        current = std::max(target, current - delta);
    }
    return current;
}

float easeOutCubic(float t) {
    float inv = 1.0f - t;
    return 1.0f - inv * inv * inv;
}

int countVisible(const std::vector<NUIDropdownItem>* items) {
    if (!items) {
        return 0;
    }
    int count = 0;
    for (const auto& item : *items) {
        if (item.visible) {
            ++count;
        }
    }
    return count;
}
} // namespace

NUIDropdownContainer::NUIDropdownContainer() {
    setLayer(NUILayer::Dropdown);
    setVisible(false);
    setEnabled(false);
    updateThemeColors();
}

NUIDropdownContainer::~NUIDropdownContainer() = default;

void NUIDropdownContainer::show(const std::shared_ptr<NUIDropdown>& owner,
                                const std::vector<NUIDropdownItem>* items,
                                int selectedIndex,
                                int maxVisibleItems,
                                const NUIRect& anchorBounds,
                                const NUIRect& viewportBounds) {
    owner_ = owner;
    items_ = items;
    selectedIndex_ = selectedIndex;
    hoveredIndex_ = selectedIndex;
    focusIndex_ = selectedIndex;
    maxVisibleItems_ = std::max(1, maxVisibleItems);
    anchorBounds_ = anchorBounds;
    viewportBounds_ = viewportBounds;

    setBounds(viewportBounds_);
    updateThemeColors();

    scrollOffset_ = 0.0f;
    maxScrollOffset_ = 0.0f;

    bool reopening = isVisible() && !closing_;
    if (reopening) {
        animationProgress_ = 1.0f;
        animationTarget_ = 1.0f;
        animating_ = false;
        closing_ = false;
    } else {
        animationProgress_ = 0.0f;
        animationTarget_ = 1.0f;
        animating_ = true;
        closing_ = false;
    }

    needsLayout_ = true;

    setVisible(true);
    setEnabled(true);
    setDirty(true);

    if (!items_ || items_->empty()) {
        hoveredIndex_ = -1;
        focusIndex_ = -1;
    } else if (focusIndex_ < 0 || focusIndex_ >= static_cast<int>(items_->size()) ||
               !(*items_)[focusIndex_].visible || !(*items_)[focusIndex_].enabled) {
        focusIndex_ = nextSelectableIndex(-1, 1);
    }
}

void NUIDropdownContainer::beginClose() {
    animationTarget_ = 0.0f;
    animating_ = true;
    closing_ = true;
    setDirty(true);
}

void NUIDropdownContainer::requestRelayout() {
    needsLayout_ = true;
    setDirty(true);
}

void NUIDropdownContainer::onRender(NUIRenderer& renderer) {
    if (!isVisible() || !items_ || items_->empty()) {
        return;
    }

    layoutIfNeeded(renderer);

    float eased = easeOut(animationProgress_);
    if (eased <= 0.0f) {
        return;
    }

    NUIRect bounds = listBounds_;
    bounds.height = std::max(1.0f, listBounds_.height * eased);
    if (!openBelow_) {
        bounds.y = listBounds_.y + listBounds_.height - bounds.height;
    }

    float slideOffset = (1.0f - eased) * 12.0f * (openBelow_ ? 1.0f : -1.0f);
    bounds.y -= slideOffset;

    NUIColor shadowColor = NUIColor(0.0f, 0.0f, 0.0f, kShadowOpacity * eased);
    renderer.drawShadow(bounds, 0.0f, kShadowOffsetY, kShadowBlur, shadowColor);

    NUIColor background = listBackground_.withAlpha(listBackground_.a * eased);
    NUIColor border = listBorder_.withAlpha(listBorder_.a * eased);

    renderer.fillRoundedRect(bounds, cornerRadius_, background);
    renderer.strokeRoundedRect(bounds, cornerRadius_, 1.0f, border);

    NUIRect clipBounds = bounds;
    clipBounds.x += 0.5f;
    clipBounds.width -= 1.0f;
    renderer.setClipRect(clipBounds);

    float contentTop = listBounds_.y + verticalPadding_ - scrollOffset_;
    float contentLeft = listBounds_.x + horizontalPadding_;
    float contentWidth = listBounds_.width - horizontalPadding_ * 2.0f;

    int visibleIndex = 0;
    for (size_t i = 0; i < items_->size(); ++i) {
        const auto& item = (*items_)[i];
        if (!item.visible) {
            continue;
        }

        float itemY = contentTop + visibleIndex * itemHeight_;
        NUIRect itemBounds(contentLeft, itemY, contentWidth, itemHeight_);

        if (itemBounds.bottom() < bounds.y || itemBounds.y > bounds.bottom()) {
            ++visibleIndex;
            continue;
        }

        NUIColor itemBg = itemBackground_;
        NUIColor textColor = itemText_;

        if (static_cast<int>(i) == selectedIndex_) {
            itemBg = NUIColor::lerp(itemBg, itemHover_, 0.75f);
            textColor = itemHoverText_;
        }

        if (static_cast<int>(i) == hoveredIndex_) {
            itemBg = NUIColor::lerp(itemBg, itemHover_, 1.0f);
            textColor = itemHoverText_;
        }

        if (!item.enabled) {
            textColor = itemDisabledText_;
            itemBg = itemBg.withAlpha(itemBg.a * 0.6f);
        }

        renderer.fillRoundedRect(itemBounds, kItemCornerRadius, itemBg.withAlpha(itemBg.a * eased));
        renderer.drawTextCentered(item.text, itemBounds, kListFontSize, textColor.withAlpha(textColor.a * eased));

        ++visibleIndex;
    }

    renderer.clearClipRect();

    if (maxScrollOffset_ > 1.0f) {
        float availableHeight = bounds.height - verticalPadding_ * 2.0f;
        float thumbHeight = std::max(24.0f, availableHeight * (availableHeight / (availableHeight + maxScrollOffset_)));
        float thumbTravel = availableHeight - thumbHeight;
        float thumbOffset = (maxScrollOffset_ > 0.0f) ? (scrollOffset_ / maxScrollOffset_) * thumbTravel : 0.0f;
        float thumbY = bounds.y + verticalPadding_ + thumbOffset;
        float thumbX = bounds.x + bounds.width - kScrollBarWidth - 4.0f;

        NUIColor trackColor = NUIColor(1.0f, 1.0f, 1.0f, 0.06f * eased);
        NUIColor thumbColor = NUIColor(1.0f, 1.0f, 1.0f, 0.35f * eased);

        NUIRect trackBounds(thumbX, bounds.y + verticalPadding_, kScrollBarWidth, availableHeight);
        renderer.fillRoundedRect(trackBounds, kScrollBarRadius, trackColor);

        NUIRect thumbBounds(thumbX, thumbY, kScrollBarWidth, thumbHeight);
        renderer.fillRoundedRect(thumbBounds, kScrollBarRadius, thumbColor);
    }
}

void NUIDropdownContainer::onUpdate(double deltaTime) {
    if (!isVisible() && !animating_) {
        return;
    }

    float step = static_cast<float>(deltaTime) * kAnimationSpeed;
    float previous = animationProgress_;
    animationProgress_ = approach(animationProgress_, animationTarget_, step);
    animationProgress_ = clamp01(animationProgress_);

    if (animationProgress_ != previous) {
        setDirty(true);
    }

    if (animating_ && std::abs(animationProgress_ - animationTarget_) <= 0.001f) {
        animationProgress_ = animationTarget_;
        animating_ = false;
        if (closing_) {
            setVisible(false);
            setEnabled(false);
            closing_ = false;
        }
    }
}

bool NUIDropdownContainer::onMouseEvent(const NUIMouseEvent& event) {
    if (!isVisible() || !items_) {
        return false;
    }

    bool insideList = listBounds_.contains(event.position);

    if (event.wheelDelta != 0.0f && insideList) {
        scrollOffset_ -= event.wheelDelta * kScrollSpeed;
        scrollOffset_ = std::clamp(scrollOffset_, 0.0f, maxScrollOffset_);
        setDirty(true);
        return true;
    }

    if (event.pressed && event.button == NUIMouseButton::Left) {
        if (insideList) {
            int index = hitTestItem(event.position);
            if (index >= 0) {
                if (items_->at(static_cast<size_t>(index)).enabled) {
                    notifySelection(index);
                }
                return true;
            }
        } else {
            notifyClose();
        }
    }

    if (insideList) {
        int index = hitTestItem(event.position);
        setHoveredIndex(index);
        return index >= 0;
    }

    setHoveredIndex(-1);
    return false;
}

bool NUIDropdownContainer::onKeyEvent(const NUIKeyEvent& event) {
    if (!isVisible() || !items_ || !event.pressed) {
        return false;
    }

    if (event.keyCode == NUIKeyCode::Escape) {
        notifyClose();
        return true;
    }

    if (event.keyCode == NUIKeyCode::Enter || event.keyCode == NUIKeyCode::Space) {
        if (focusIndex_ >= 0 && focusIndex_ < static_cast<int>(items_->size())) {
            if ((*items_)[focusIndex_].enabled) {
                notifySelection(focusIndex_);
                return true;
            }
        }
    }

    if (event.keyCode == NUIKeyCode::Down || event.keyCode == NUIKeyCode::Up) {
        int direction = event.keyCode == NUIKeyCode::Down ? 1 : -1;
        int start = focusIndex_;
        if (start < 0) {
            start = direction > 0 ? -1 : static_cast<int>(items_->size());
        }
        int next = nextSelectableIndex(start, direction);
        if (next != focusIndex_ && next >= 0) {
            focusIndex_ = next;
            setHoveredIndex(next);
            ensureSelectionVisible();
            return true;
        }
    }

    return false;
}

void NUIDropdownContainer::layoutIfNeeded(NUIRenderer& renderer) {
    if (!needsLayout_ || !items_) {
        return;
    }

    updateThemeColors();
    computeListBounds(renderer);
    ensureSelectionVisible();
    needsLayout_ = false;
}

void NUIDropdownContainer::updateThemeColors() {
    auto& themeManager = NUIThemeManager::getInstance();
    listBackground_ = themeManager.getColor("dropdown.list.background");
    listBorder_ = themeManager.getColor("dropdown.list.border");
    itemBackground_ = themeManager.getColor("dropdown.item.background");
    itemHover_ = themeManager.getColor("dropdown.item.hover");
    itemSelected_ = themeManager.getColor("dropdown.hover");
    itemText_ = themeManager.getColor("dropdown.item.text");
    itemHoverText_ = themeManager.getColor("dropdown.item.hoverText");
    itemDisabledText_ = themeManager.getColor("dropdown.item.disabled");
    dividerColor_ = themeManager.getColor("dropdown.item.divider");
}

void NUIDropdownContainer::computeListBounds(NUIRenderer& renderer) {
    int visibleCount = countVisible(items_);
    if (visibleCount == 0) {
        listBounds_ = NUIRect(anchorBounds_.x, anchorBounds_.bottom(), anchorBounds_.width, itemHeight_);
        return;
    }

    float maxTextWidth = 0.0f;
    for (const auto& item : *items_) {
        if (!item.visible) {
            continue;
        }
        NUISize size = renderer.measureText(item.text, kListFontSize);
        maxTextWidth = std::max(maxTextWidth, size.width);
    }

    float desiredWidth = std::max(anchorBounds_.width, maxTextWidth + horizontalPadding_ * 2.0f);
    float maxWidth = std::max(120.0f, viewportBounds_.width - listMargin_ * 2.0f);
    float width = std::min(desiredWidth, maxWidth);

    float contentHeight = visibleCount * itemHeight_ + verticalPadding_ * 2.0f;
    float limitHeight = maxVisibleItems_ > 0
        ? std::min(contentHeight, maxVisibleItems_ * itemHeight_ + verticalPadding_ * 2.0f)
        : contentHeight;
    float heightCap = std::min(maxListHeight_, viewportBounds_.height - listMargin_ * 2.0f);
    float maxHeight = std::max(itemHeight_ + verticalPadding_ * 2.0f,
                               std::min(limitHeight, heightCap));
    float height = std::min(contentHeight, maxHeight);

    float spaceBelow = viewportBounds_.y + viewportBounds_.height - (anchorBounds_.y + anchorBounds_.height);
    float spaceAbove = anchorBounds_.y - viewportBounds_.y;
    openBelow_ = (spaceBelow >= height) || (spaceBelow >= spaceAbove);

    float x = std::clamp(anchorBounds_.x, viewportBounds_.x + listMargin_,
                         viewportBounds_.x + viewportBounds_.width - width - listMargin_);
    float y = openBelow_ ? anchorBounds_.y + anchorBounds_.height + 2.0f
                         : anchorBounds_.y - height - 2.0f;

    y = std::clamp(y, viewportBounds_.y + listMargin_,
                   viewportBounds_.y + viewportBounds_.height - height - listMargin_);

    listBounds_ = NUIRect(x, y, width, height);

    maxScrollOffset_ = std::max(0.0f, contentHeight - height);
    scrollOffset_ = std::clamp(scrollOffset_, 0.0f, maxScrollOffset_);
}

void NUIDropdownContainer::ensureSelectionVisible() {
    if (!items_ || listBounds_.height <= 0.0f) {
        return;
    }

    float viewportHeight = listBounds_.height - verticalPadding_ * 2.0f;
    if (viewportHeight <= 0.0f) {
        return;
    }

    int targetIndex = focusIndex_ >= 0 ? focusIndex_ : selectedIndex_;
    if (targetIndex < 0) {
        return;
    }

    float itemTop = verticalPadding_;
    int visibleIndex = 0;
    for (size_t i = 0; i < items_->size(); ++i) {
        if (!(*items_)[i].visible) {
            continue;
        }

        if (static_cast<int>(i) == targetIndex) {
            float top = itemTop;
            float bottom = itemTop + itemHeight_;

            if (top < scrollOffset_) {
                scrollOffset_ = top;
            } else if (bottom > scrollOffset_ + viewportHeight) {
                scrollOffset_ = bottom - viewportHeight;
            }
            break;
        }

        itemTop += itemHeight_;
        ++visibleIndex;
    }

    scrollOffset_ = std::clamp(scrollOffset_, 0.0f, maxScrollOffset_);
}

void NUIDropdownContainer::setHoveredIndex(int index) {
    if (hoveredIndex_ == index) {
        return;
    }

    hoveredIndex_ = index;
    setDirty(true);

    if (auto owner = owner_.lock()) {
        owner->handleItemHovered(index);
    }
}

int NUIDropdownContainer::hitTestItem(const NUIPoint& position) const {
    if (!listBounds_.contains(position)) {
        return -1;
    }

    float relativeY = position.y - (listBounds_.y + verticalPadding_) + scrollOffset_;
    if (relativeY < 0.0f) {
        return -1;
    }

    int visibleIndex = static_cast<int>(relativeY / itemHeight_);
    int currentIndex = 0;
    for (size_t i = 0; i < items_->size(); ++i) {
        if (!(*items_)[i].visible) {
            continue;
        }
        if (currentIndex == visibleIndex) {
            return static_cast<int>(i);
        }
        ++currentIndex;
    }
    return -1;
}

int NUIDropdownContainer::nextSelectableIndex(int start, int direction) const {
    if (!items_) {
        return -1;
    }

    int index = start;
    for (size_t step = 0; step < items_->size(); ++step) {
        index += direction;
        if (index < 0 || index >= static_cast<int>(items_->size())) {
            break;
        }
        const auto& item = (*items_)[index];
        if (item.visible && item.enabled) {
            return index;
        }
    }
    return start;
}

float NUIDropdownContainer::easeOut(float t) const {
    return easeOutCubic(clamp01(t));
}

void NUIDropdownContainer::notifySelection(int index) {
    selectedIndex_ = index;
    if (auto owner = owner_.lock()) {
        owner->handleItemSelected(index);
    }
}

void NUIDropdownContainer::notifyClose() {
    if (auto owner = owner_.lock()) {
        owner->closeDropdown();
    }
}

} // namespace NomadUI

