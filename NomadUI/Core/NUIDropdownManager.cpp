#include "NUIDropdownManager.h"
#include "NUIDropdownContainer.h"
#include <algorithm>
#include <iostream>

namespace NomadUI {

NUIDropdownManager& NUIDropdownManager::getInstance() {
    static NUIDropdownManager instance;
    return instance;
}

void NUIDropdownManager::registerDropdown(std::shared_ptr<NUIDropdown> dropdown) {
    if (!dropdown) return;
    
    // Remove if already registered
    registeredDropdowns_.erase(
        std::remove_if(registeredDropdowns_.begin(), registeredDropdowns_.end(),
            [&dropdown](const std::weak_ptr<NUIDropdown>& weak) {
                return weak.lock() == dropdown;
            }),
        registeredDropdowns_.end()
    );
    
    // Add to registered list
    registeredDropdowns_.push_back(dropdown);
}

void NUIDropdownManager::unregisterDropdown(std::shared_ptr<NUIDropdown> dropdown) {
    if (!dropdown) return;
    
    // Remove from registered list
    registeredDropdowns_.erase(
        std::remove_if(registeredDropdowns_.begin(), registeredDropdowns_.end(),
            [&dropdown](const std::weak_ptr<NUIDropdown>& weak) {
                return weak.lock() == dropdown;
            }),
        registeredDropdowns_.end()
    );
    
    // Close if this was the open dropdown
    if (openDropdown_ == dropdown) {
        openDropdown_.reset();
    }
}

void NUIDropdownManager::openDropdown(std::shared_ptr<NUIDropdown> dropdown) {
    if (!dropdown) return;
    
    std::cout << "[DropdownManager] Opening dropdown: " << dropdown->getId() << std::endl;
    
    // Close any currently open dropdown
    closeOtherDropdowns(dropdown);
    
    // Hide all other dropdown containers
    for (auto& weakDropdown : registeredDropdowns_) {
        if (auto otherDropdown = weakDropdown.lock()) {
            if (otherDropdown != dropdown) {
                auto container = otherDropdown->getContainer();
                if (container) {
                    container->setVisible(false);
                }
            }
        }
    }
    
    // Set this as the open dropdown
    openDropdown_ = dropdown;
    std::cout << "[DropdownManager] Set open dropdown to: " << dropdown->getId() << std::endl;
    
    // Ensure the new dropdown has the highest Z-order
    auto container = dropdown->getContainer();
    if (container) {
        container->setLayer(NUILayer::Dropdown);
        std::cout << "[DropdownManager] Set container layer to Dropdown" << std::endl;
    } else {
        std::cout << "[DropdownManager] ERROR: No container found for dropdown!" << std::endl;
    }
    
    // Update positioning for all dropdowns
    updateDropdownPositions();
}

void NUIDropdownManager::closeAllDropdowns() {
    if (openDropdown_) {
        // Hide the container before closing
        auto container = openDropdown_->getContainer();
        if (container) {
            container->setVisible(false);
        }
        openDropdown_->closeDropdown();
        openDropdown_.reset();
    }
}

void NUIDropdownManager::closeDropdown(std::shared_ptr<NUIDropdown> dropdown) {
    if (openDropdown_ == dropdown) {
        // Hide the container before closing
        auto container = openDropdown_->getContainer();
        if (container) {
            container->setVisible(false);
        }
        openDropdown_.reset();
    }
}

bool NUIDropdownManager::isAnyDropdownOpen() const {
    return openDropdown_ != nullptr;
}

std::shared_ptr<NUIDropdown> NUIDropdownManager::getOpenDropdown() const {
    return openDropdown_;
}

void NUIDropdownManager::updateDropdownPositions() {
    if (!openDropdown_) {
        std::cout << "No open dropdown to position" << std::endl;
        return;
    }
    
    // Get the container from the open dropdown
    auto container = openDropdown_->getContainer();
    if (!container) {
        std::cout << "No container found for open dropdown" << std::endl;
        return;
    }
    
    std::cout << "Updating dropdown positions with global coordinate fix" << std::endl;
    
    // Set proper Z-order - higher than all other UI
    container->setLayer(NUILayer::Dropdown);
    
    // Get global bounds - this now correctly walks up the parent chain
    NUIRect globalBounds = openDropdown_->getGlobalBounds();
    
    // Calculate container height (estimate based on items)
    float containerHeight = std::min(200.0f, openDropdown_->getItems().size() * 32.0f);
    
    // Get screen space limits
    float screenHeight = availableSpace_.height;
    
    // Debug: Print global vs local bounds
    std::cout << "[Dropdown] Global bounds: (" << globalBounds.x << "," << globalBounds.y 
              << ") " << globalBounds.width << "x" << globalBounds.height << std::endl;
    std::cout << "[Dropdown] Available space: (" << availableSpace_.x << "," << availableSpace_.y 
              << ") " << availableSpace_.width << "x" << availableSpace_.height << std::endl;
    
    // Calculate drop position - try below first
    float containerY = globalBounds.y + globalBounds.height;
    
    // Check if there's enough room below, if not flip upward
    if (containerY + containerHeight > screenHeight) {
        containerY = globalBounds.y - containerHeight;
        std::cout << "[Dropdown] Not enough room below, flipping upward" << std::endl;
    } else {
        std::cout << "[Dropdown] Dropping below source" << std::endl;
    }
    
    // Ensure container doesn't go off-screen
    containerY = std::max(0.0f, std::min(containerY, screenHeight - containerHeight));
    
    // Set the global bounds for the container
    NUIRect containerBounds = {
        globalBounds.x,           // Same X as source
        containerY,               // Calculated Y
        globalBounds.width,       // Same width as source
        containerHeight           // Calculated height
    };
    
    container->setSourceBounds(containerBounds);
    container->setAvailableSpace(availableSpace_);
    container->setVisible(true);
    
    std::cout << "[Dropdown] Container positioned at: (" << containerBounds.x << "," << containerBounds.y 
              << ") " << containerBounds.width << "x" << containerBounds.height << std::endl;
}

void NUIDropdownManager::setAvailableSpace(const NUIRect& space) {
    availableSpace_ = space;
    updateDropdownPositions();
}

void NUIDropdownManager::closeOtherDropdowns(std::shared_ptr<NUIDropdown> currentDropdown) {
    // Close any other open dropdowns
    for (auto& weak : registeredDropdowns_) {
        auto dropdown = weak.lock();
        if (dropdown && dropdown != currentDropdown && dropdown->isOpen()) {
            dropdown->closeDropdown();
        }
    }
}

} // namespace NomadUI
