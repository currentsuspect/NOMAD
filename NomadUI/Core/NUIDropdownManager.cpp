#include "NUIDropdownManager.h"
#include "NUIDropdown.h"
#include "NUIDropdownContainer.h"
#include "NUIComponent.h"

#include <algorithm>
#include <stdexcept>

namespace NomadUI {

NUIDropdownManager& NUIDropdownManager::getInstance() {
    static NUIDropdownManager instance;
    return instance;
}

void NUIDropdownManager::registerDropdown(const std::shared_ptr<NUIDropdown>& dropdown) {
    if (!dropdown) {
        return;
    }

    pruneExpired();

    auto it = std::find_if(dropdowns_.begin(), dropdowns_.end(),
        [&dropdown](const std::weak_ptr<NUIDropdown>& weak) {
            return weak.lock() == dropdown;
        });

    if (it == dropdowns_.end()) {
        dropdowns_.push_back(dropdown);
    }
}

void NUIDropdownManager::unregisterDropdown(const std::shared_ptr<NUIDropdown>& dropdown) {
    if (!dropdown) {
        return;
    }

    dropdowns_.erase(std::remove_if(dropdowns_.begin(), dropdowns_.end(),
        [&dropdown](const std::weak_ptr<NUIDropdown>& weak) {
            return weak.lock() == dropdown || weak.expired();
        }), dropdowns_.end());

    if (auto current = openDropdown_.lock()) {
        if (current == dropdown) {
            closeDropdown(dropdown);
        }
    }
}

void NUIDropdownManager::openDropdown(const std::shared_ptr<NUIDropdown>& dropdown) {
    if (!dropdown) {
        return;
    }

    pruneExpired();

    if (auto current = openDropdown_.lock()) {
        if (current != dropdown) {
            closeDropdown(current);
        }
    }

    auto container = dropdown->getContainer();
    if (!container) {
        return;
    }

    auto root = resolveRoot(dropdown);
    if (!root) {
        return;
    }

    if (container->getParent() != root.get()) {
        root->addChild(container);
    }

    NUIRect viewport = computeViewportBounds(root);
    NUIRect anchor = computeGlobalBounds(*dropdown);

    container->show(dropdown, &dropdown->items_, dropdown->selectedIndex_, dropdown->maxVisibleItems_, anchor, viewport);
    dropdown->applyOpenState(true);

    openDropdown_ = dropdown;
}

void NUIDropdownManager::closeDropdown(const std::shared_ptr<NUIDropdown>& dropdown) {
    if (!dropdown) {
        return;
    }

    auto container = dropdown->getContainer();
    if (container) {
        container->beginClose();
    }

    dropdown->applyOpenState(false);

    if (auto current = openDropdown_.lock()) {
        if (current == dropdown) {
            openDropdown_.reset();
        }
    }
}

void NUIDropdownManager::closeAllDropdowns() {
    if (auto current = openDropdown_.lock()) {
        closeDropdown(current);
    }
}

void NUIDropdownManager::refreshOpenDropdown() {
    if (auto dropdown = openDropdown_.lock()) {
        auto container = dropdown->getContainer();
        auto root = resolveRoot(dropdown);
        if (container && root) {
            NUIRect viewport = computeViewportBounds(root);
            NUIRect anchor = computeGlobalBounds(*dropdown);
            container->show(dropdown, &dropdown->items_, dropdown->selectedIndex_, dropdown->maxVisibleItems_, anchor, viewport);
        }
    }
}

void NUIDropdownManager::pruneExpired() {
    dropdowns_.erase(std::remove_if(dropdowns_.begin(), dropdowns_.end(),
        [](const std::weak_ptr<NUIDropdown>& weak) { return weak.expired(); }), dropdowns_.end());
}

std::shared_ptr<NUIComponent> NUIDropdownManager::resolveRoot(const std::shared_ptr<NUIDropdown>& dropdown) const {
    if (!dropdown) {
        return nullptr;
    }

    NUIComponent* current = dropdown.get();
    while (current && current->getParent()) {
        current = current->getParent();
    }

    if (!current) {
        return nullptr;
    }

    try {
        return current->shared_from_this();
    } catch (const std::bad_weak_ptr&) {
        return nullptr;
    }
}

NUIRect NUIDropdownManager::computeGlobalBounds(const NUIDropdown& dropdown) const {
    NUIRect bounds = dropdown.getBounds();
    const NUIComponent* parent = dropdown.getParent();
    while (parent) {
        NUIRect parentBounds = parent->getBounds();
        bounds.x += parentBounds.x;
        bounds.y += parentBounds.y;
        parent = parent->getParent();
    }
    return bounds;
}

NUIRect NUIDropdownManager::computeViewportBounds(const std::shared_ptr<NUIComponent>& root) const {
    if (!root) {
        return NUIRect();
    }

    NUIRect bounds = root->getBounds();
    if (bounds.width <= 0.0f || bounds.height <= 0.0f) {
        bounds.x = 0.0f;
        bounds.y = 0.0f;
        bounds.width = std::max(root->getWidth(), 0.0f);
        bounds.height = std::max(root->getHeight(), 0.0f);
    }
    return bounds;
}

} // namespace NomadUI

