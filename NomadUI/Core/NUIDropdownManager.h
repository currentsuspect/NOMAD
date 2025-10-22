#pragma once

#include "NUIComponent.h"
#include "NUIDropdown.h"
#include <memory>
#include <vector>

namespace NomadUI {

/**
 * Global dropdown manager that ensures only one dropdown is open at a time
 * and handles proper Z-ordering and positioning to prevent clashing.
 */
class NUIDropdownManager {
public:
    static NUIDropdownManager& getInstance();
    
    // ========================================================================
    // Dropdown Registration
    // ========================================================================
    
    void registerDropdown(std::shared_ptr<NUIDropdown> dropdown);
    void unregisterDropdown(std::shared_ptr<NUIDropdown> dropdown);
    
    // ========================================================================
    // Dropdown Control
    // ========================================================================
    
    void openDropdown(std::shared_ptr<NUIDropdown> dropdown);
    void closeAllDropdowns();
    void closeDropdown(std::shared_ptr<NUIDropdown> dropdown);
    
    // ========================================================================
    // State Management
    // ========================================================================
    
    bool isAnyDropdownOpen() const;
    std::shared_ptr<NUIDropdown> getOpenDropdown() const;
    
    // ========================================================================
    // Positioning & Z-Order
    // ========================================================================
    
    void updateDropdownPositions();
    void setAvailableSpace(const NUIRect& space);
    
private:
    NUIDropdownManager() = default;
    ~NUIDropdownManager() = default;
    
    std::vector<std::weak_ptr<NUIDropdown>> registeredDropdowns_;
    std::shared_ptr<NUIDropdown> openDropdown_;
    NUIRect availableSpace_ = {0, 0, 1280, 720};
    
    void closeOtherDropdowns(std::shared_ptr<NUIDropdown> currentDropdown);
};

} // namespace NomadUI
