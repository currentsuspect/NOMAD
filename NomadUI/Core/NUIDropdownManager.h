#pragma once

#include "NUITypes.h"
#include <memory>
#include <vector>

namespace NomadUI {

class NUIDropdown;
class NUIDropdownContainer;
class NUIComponent;

class NUIDropdownManager {
public:
    static NUIDropdownManager& getInstance();

    void registerDropdown(const std::shared_ptr<NUIDropdown>& dropdown);
    void unregisterDropdown(const std::shared_ptr<NUIDropdown>& dropdown);

    void openDropdown(const std::shared_ptr<NUIDropdown>& dropdown);
    void closeDropdown(const std::shared_ptr<NUIDropdown>& dropdown);
    void closeAllDropdowns();
    void refreshOpenDropdown();

private:
    NUIDropdownManager() = default;

    void pruneExpired();
    std::shared_ptr<NUIComponent> resolveRoot(const std::shared_ptr<NUIDropdown>& dropdown) const;
    NUIRect computeGlobalBounds(const NUIDropdown& dropdown) const;
    NUIRect computeViewportBounds(const std::shared_ptr<NUIComponent>& root) const;

    std::vector<std::weak_ptr<NUIDropdown>> dropdowns_;
    std::weak_ptr<NUIDropdown> openDropdown_;
};

} // namespace NomadUI

