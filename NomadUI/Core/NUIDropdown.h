#pragma once

#include "NUIComponent.h"
#include "NUITypes.h"
#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace NomadUI {

class NUIRenderer;
class NUIDropdownContainer;
class NUIDropdownManager;

struct NUIDropdownItem {
    std::string text;
    int value = 0;
    bool enabled = true;
    bool visible = true;
};

class NUIDropdown : public NUIComponent {
public:
    NUIDropdown();
    ~NUIDropdown() override;

    void addItem(const std::string& text, int value);
    void addItem(const NUIDropdownItem& item);
    void setItems(const std::vector<NUIDropdownItem>& items);
    void clear();

    void setSelectedIndex(int index);
    int getSelectedIndex() const { return selectedIndex_; }
    void setSelectedValue(int value);
    int getSelectedValue() const;
    std::string getSelectedText() const;

    void setPlaceholderText(const std::string& text);
    void setOnSelectionChanged(std::function<void(int, int, const std::string&)> callback);
    void setMaxVisibleItems(int count);

    void onRender(NUIRenderer& renderer) override;
    void onUpdate(double deltaTime) override;
    bool onMouseEvent(const NUIMouseEvent& event) override;
    bool onKeyEvent(const NUIKeyEvent& event) override;
    void onFocusLost() override;

    bool isOpen() const { return isOpen_; }
    std::shared_ptr<NUIDropdownContainer> getContainer() const { return container_; }
    const std::vector<NUIDropdownItem>& getItems() const { return items_; }

private:
    void ensureRegistration();
    void toggleDropdown();
    void openDropdown();
    void closeDropdown();
    void applyOpenState(bool open);
    void refreshContainerLayout();
    void handleItemSelected(int index);
    void handleItemHovered(int index);
    void updateArrowAnimation(double deltaTime);
    void updateButtonState();
    std::string getDisplayText() const;

    std::vector<NUIDropdownItem> items_;
    int selectedIndex_ = -1;
    int hoveredIndex_ = -1;
    int maxVisibleItems_ = 8;

    std::string placeholderText_;
    std::function<void(int, int, const std::string&)> onSelectionChanged_;

    std::shared_ptr<NUIDropdownContainer> container_;
    bool registeredWithManager_ = false;
    bool isOpen_ = false;
    bool pressedInside_ = false;
    bool pointerInside_ = false;

    float arrowProgress_ = 0.0f;
    float arrowTarget_ = 0.0f;
    float hoverProgress_ = 0.0f;

    NUIColor backgroundColor_;
    NUIColor borderColor_;
    NUIColor textColor_;
    NUIColor arrowColor_;
    NUIColor hoverColor_;
    NUIColor disabledColor_;
    NUIColor focusColor_;

    float cornerRadius_ = 6.0f;
    float borderThickness_ = 1.0f;

    friend class NUIDropdownContainer;
    friend class NUIDropdownManager;
};

} // namespace NomadUI

