#include "NUIDropdown.h"
#include "NUIDropdownContainer.h"
#include "NUIDropdownManager.h"
#include "NUIThemeSystem.h"
#include "../Graphics/NUIRenderer.h"

#include <algorithm>
#include <cmath>

namespace NomadUI {
namespace {
constexpr float kArrowAnimationSpeed = 10.0f;
constexpr float kHoverAnimationSpeed = 12.0f;
constexpr float kButtonFontSize = 14.0f;

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
} // namespace

NUIDropdown::NUIDropdown() {
    setLayer(NUILayer::Content);

    auto& themeManager = NUIThemeManager::getInstance();
    backgroundColor_ = themeManager.getColor("dropdown.background");
    borderColor_ = themeManager.getColor("dropdown.border");
    textColor_ = themeManager.getColor("dropdown.text");
    arrowColor_ = themeManager.getColor("dropdown.arrow");
    hoverColor_ = themeManager.getColor("dropdown.hover");
    focusColor_ = themeManager.getColor("dropdown.focus");
    disabledColor_ = themeManager.getColor("textDisabled");

    container_ = std::make_shared<NUIDropdownContainer>();
    container_->setVisible(false);
    container_->setEnabled(false);
}

NUIDropdown::~NUIDropdown() {
    if (container_) {
        container_->close();
        container_.reset();
    }
}

void NUIDropdown::ensureRegistration() {
    if (registeredWithManager_) {
        return;
    }

    if (auto self = std::static_pointer_cast<NUIDropdown>(shared_from_this())) {
        NUIDropdownManager::getInstance().registerDropdown(self);
        registeredWithManager_ = true;
    }
}

void NUIDropdown::addItem(const std::string& text, int value) {
    addItem(NUIDropdownItem{text, value, true, true});
}

void NUIDropdown::addItem(const NUIDropdownItem& item) {
    items_.push_back(item);
    setDirty(true);
    refreshContainerLayout();
}

void NUIDropdown::setItems(const std::vector<NUIDropdownItem>& items) {
    items_ = items;
    selectedIndex_ = std::clamp(selectedIndex_, -1, static_cast<int>(items_.size()) - 1);
    setDirty(true);
    refreshContainerLayout();
}

void NUIDropdown::clear() {
    items_.clear();
    selectedIndex_ = -1;
    hoveredIndex_ = -1;
    setDirty(true);
    refreshContainerLayout();
}

void NUIDropdown::setSelectedIndex(int index) {
    if (index < 0 || index >= static_cast<int>(items_.size())) {
        selectedIndex_ = -1;
    } else if (items_[index].visible && items_[index].enabled) {
        selectedIndex_ = index;
    }

    setDirty(true);
    refreshContainerLayout();
}

void NUIDropdown::setSelectedValue(int value) {
    for (size_t i = 0; i < items_.size(); ++i) {
        if (items_[i].value == value && items_[i].visible && items_[i].enabled) {
            setSelectedIndex(static_cast<int>(i));
            return;
        }
    }
}

int NUIDropdown::getSelectedValue() const {
    if (selectedIndex_ >= 0 && selectedIndex_ < static_cast<int>(items_.size())) {
        return items_[selectedIndex_].value;
    }
    return 0;
}

std::string NUIDropdown::getSelectedText() const {
    if (selectedIndex_ >= 0 && selectedIndex_ < static_cast<int>(items_.size())) {
        return items_[selectedIndex_].text;
    }
    return std::string();
}

void NUIDropdown::setPlaceholderText(const std::string& text) {
    placeholderText_ = text;
    setDirty(true);
}

void NUIDropdown::setOnSelectionChanged(std::function<void(int, int, const std::string&)> callback) {
    onSelectionChanged_ = std::move(callback);
}

void NUIDropdown::setMaxVisibleItems(int count) {
    maxVisibleItems_ = std::max(1, count);
    refreshContainerLayout();
}

void NUIDropdown::onRender(NUIRenderer& renderer) {
    ensureRegistration();
    updateButtonState();

    NUIRect bounds = getBounds();

    NUIColor baseColor = backgroundColor_;
    NUIColor borderColor = borderColor_;

    float hoverMix = easeOutCubic(clamp01(hoverProgress_));
    if (pointerInside_) {
        baseColor = NUIColor::lerp(baseColor, hoverColor_, hoverMix);
    }

    if (isOpen_) {
        baseColor = NUIColor::lerp(baseColor, focusColor_, 0.35f);
        borderColor = focusColor_;
    }

    if (!isEnabled()) {
        baseColor = baseColor.withAlpha(baseColor.a * 0.7f);
        borderColor = borderColor.withAlpha(borderColor.a * 0.6f);
    }

    renderer.fillRoundedRect(bounds, cornerRadius_, baseColor);
    renderer.strokeRoundedRect(bounds, cornerRadius_, borderThickness_, borderColor);

    bool usingPlaceholder = selectedIndex_ < 0 || selectedIndex_ >= static_cast<int>(items_.size());
    std::string displayText = usingPlaceholder ? placeholderText_ : getDisplayText();
    NUIColor textColor = isEnabled() ? textColor_ : disabledColor_;
    if (usingPlaceholder && !placeholderText_.empty()) {
        textColor = textColor.withAlpha(textColor.a * 0.75f);
    }

    renderer.drawTextCentered(displayText, bounds, kButtonFontSize, textColor);

    float arrowSize = 6.0f;
    float arrowCenterX = bounds.x + bounds.width - 18.0f;
    float arrowCenterY = bounds.y + bounds.height * 0.5f;
    float progress = clamp01(arrowProgress_);

    NUIPoint downTip(arrowCenterX, arrowCenterY + arrowSize);
    NUIPoint downLeft(arrowCenterX - arrowSize, arrowCenterY - arrowSize);
    NUIPoint downRight(arrowCenterX + arrowSize, arrowCenterY - arrowSize);

    NUIPoint upTip(arrowCenterX, arrowCenterY - arrowSize);
    NUIPoint upLeft(arrowCenterX - arrowSize, arrowCenterY + arrowSize);
    NUIPoint upRight(arrowCenterX + arrowSize, arrowCenterY + arrowSize);

    NUIPoint p1(
        downLeft.x + (upLeft.x - downLeft.x) * progress,
        downLeft.y + (upLeft.y - downLeft.y) * progress);
    NUIPoint p2(
        downRight.x + (upRight.x - downRight.x) * progress,
        downRight.y + (upRight.y - downRight.y) * progress);
    NUIPoint p3(
        downTip.x + (upTip.x - downTip.x) * progress,
        downTip.y + (upTip.y - downTip.y) * progress);

    renderer.drawLine(p1, p2, 1.8f, arrowColor_);
    renderer.drawLine(p2, p3, 1.8f, arrowColor_);
    renderer.drawLine(p3, p1, 1.8f, arrowColor_);
}

void NUIDropdown::onUpdate(double deltaTime) {
    updateArrowAnimation(deltaTime);

    float targetHover = pointerInside_ ? 1.0f : 0.0f;
    hoverProgress_ = approach(hoverProgress_, targetHover, static_cast<float>(deltaTime) * kHoverAnimationSpeed);

}

bool NUIDropdown::onMouseEvent(const NUIMouseEvent& event) {
    ensureRegistration();

    if (!isEnabled()) {
        return false;
    }

    NUIRect bounds = getBounds();
    bool inside = bounds.contains(event.position);
    pointerInside_ = inside;

    if (event.pressed && event.button == NUIMouseButton::Left) {
        pressedInside_ = inside;
        if (inside) {
            toggleDropdown();
            return true;
        }
    }

    if (event.released && event.button == NUIMouseButton::Left) {
        pressedInside_ = false;
    }

    return inside;
}

bool NUIDropdown::onKeyEvent(const NUIKeyEvent& event) {
    if (!isOpen_ || !container_) {
        return false;
    }

    if (container_->onKeyEvent(event)) {
        return true;
    }

    if (event.pressed) {
        if (event.keyCode == NUIKeyCode::Escape) {
            closeDropdown();
            return true;
        }
    }

    return false;
}

void NUIDropdown::onFocusLost() {
    pointerInside_ = false;
    pressedInside_ = false;
    if (isOpen_) {
        closeDropdown();
    }
}

void NUIDropdown::toggleDropdown() {
    if (isOpen_) {
        closeDropdown();
    } else {
        openDropdown();
    }
}

void NUIDropdown::openDropdown() {
    ensureRegistration();
    auto self = std::static_pointer_cast<NUIDropdown>(shared_from_this());
    NUIDropdownManager::getInstance().openDropdown(self);
}

void NUIDropdown::closeDropdown() {
    if (!isOpen_) {
        return;
    }

    if (auto self = std::static_pointer_cast<NUIDropdown>(shared_from_this())) {
        NUIDropdownManager::getInstance().closeDropdown(self);
    }
}

void NUIDropdown::applyOpenState(bool open) {
    isOpen_ = open;
    arrowTarget_ = open ? 1.0f : 0.0f;
    setDirty(true);
}

void NUIDropdown::refreshContainerLayout() {
    if (container_) {
        container_->requestRelayout();
    }

    if (isOpen_) {
        NUIDropdownManager::getInstance().refreshOpenDropdown();
    }
}

void NUIDropdown::handleItemSelected(int index) {
    if (index < 0 || index >= static_cast<int>(items_.size())) {
        return;
    }

    if (!items_[index].visible || !items_[index].enabled) {
        return;
    }

    selectedIndex_ = index;
    setDirty(true);

    if (onSelectionChanged_) {
        const auto& item = items_[index];
        onSelectionChanged_(index, item.value, item.text);
    }

    closeDropdown();
}

void NUIDropdown::handleItemHovered(int index) {
    hoveredIndex_ = index;
    setDirty(true);
}

void NUIDropdown::updateArrowAnimation(double deltaTime) {
    float step = static_cast<float>(deltaTime) * kArrowAnimationSpeed;
    arrowProgress_ = approach(arrowProgress_, arrowTarget_, step);
}

void NUIDropdown::updateButtonState() {
    auto& themeManager = NUIThemeManager::getInstance();
    backgroundColor_ = themeManager.getColor("dropdown.background");
    borderColor_ = themeManager.getColor("dropdown.border");
    textColor_ = themeManager.getColor("dropdown.text");
    arrowColor_ = themeManager.getColor("dropdown.arrow");
    hoverColor_ = themeManager.getColor("dropdown.hover");
    focusColor_ = themeManager.getColor("dropdown.focus");
    disabledColor_ = themeManager.getColor("textDisabled");
}

std::string NUIDropdown::getDisplayText() const {
    if (selectedIndex_ >= 0 && selectedIndex_ < static_cast<int>(items_.size())) {
        return items_[selectedIndex_].text;
    }
    return std::string();
}

} // namespace NomadUI

