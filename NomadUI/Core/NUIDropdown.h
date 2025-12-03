// © 2025 Nomad Studios – All Rights Reserved. Licensed for personal & educational use only.
#ifndef NUIDROPDOWN_H
#define NUIDROPDOWN_H

#include "NUIComponent.h"
#include "NUILabel.h"
#include <vector>
#include <string>
#include <functional>
#include <optional>
#include <memory>

/**
 * Represents a single entry in a dropdown list.
 *
 * Holds the display text, an optional activation callback, an integer value,
 * an enabled flag, and an optional icon path.
 */

/**
 * Encapsulates data for a selection change event.
 *
 * Contains the newly selected item's index, associated integer value, and text.
 */

/**
 * Construct a NUIDropdown control.
 */

/**
 * Destroy the NUIDropdown control.
 */

/**
 * Add an item with the given text.
 *
 * @param text Display label for the new item.
 */

/**
 * Add an item with the given text and callback invoked when selected.
 *
 * @param text Display label for the new item.
 * @param callback Function called when the item is chosen.
 */

/**
 * Add an item with the given text and associated integer value.
 *
 * @param text Display label for the new item.
 * @param value Integer value associated with the item.
 */

/**
 * Add an item with the given text, value, and callback.
 *
 * @param text Display label for the new item.
 * @param value Integer value associated with the item.
 * @param callback Function called when the item is chosen.
 */

/**
 * Add a pre-constructed DropdownItem to the list.
 *
 * @param item DropdownItem instance to append.
 */

/**
 * Remove the item at the specified index.
 *
 * @param index Zero-based index of the item to remove.
 */

/**
 * Remove all items from the dropdown.
 */

/**
 * Alias for clear(); removes all items from the dropdown.
 */

/**
 * Set the currently selected item by unsigned index.
 *
 * If the index is out of range, selection behavior is implementation-defined.
 *
 * @param index Zero-based index to select.
 */

/**
 * Set the currently selected item by (possibly negative) integer index.
 *
 * If the index is out of range, selection behavior is implementation-defined.
 *
 * @param index Index to select.
 */

/**
 * Select the first item that has the given integer value.
 *
 * @param value Value to match for selection.
 */

/**
 * Get the currently selected index.
 *
 * @returns The zero-based index of the current selection.
 */

/**
 * Get the text of the currently selected item.
 *
 * @returns The display text of the selected item, or an empty string if none.
 */

/**
 * Get the integer value of the currently selected item.
 *
 * @returns The value associated with the selected item.
 */

/**
 * Get the currently selected item.
 *
 * @returns An optional containing the selected DropdownItem, or empty if none.
 */

/**
 * Get the number of items in the dropdown.
 *
 * @returns The count of items.
 */

/**
 * Access the internal list of items.
 *
 * @returns Const reference to the vector of DropdownItem.
 */

/**
 * Get the item at the specified index.
 *
 * @param index Zero-based index of the requested item.
 * @returns An optional containing the item if index is valid, or empty otherwise.
 */

/**
 * Set placeholder text shown when no item is selected.
 *
 * @param text Placeholder string to display.
 */

/**
 * Set the height, in pixels, used to render each item.
 *
 * @param height Height for each item row.
 */

/**
 * Set the maximum number of visible items before scrolling is required.
 *
 * @param count Maximum number of items to show without scrolling.
 */

/**
 * Enable or disable incremental character search within items.
 *
 * @param enabled True to enable search, false to disable.
 */

/**
 * Enable or disable open/close animation for the dropdown.
 *
 * @param enabled True to enable animation, false to disable.
 */

/**
 * Set a callback invoked when selection changes, receiving the selected index.
 *
 * @param callback Function called with the newly selected zero-based index.
 */

/**
 * Set a callback invoked when selection changes, receiving (index, value, text).
 *
 * @param callback Function called with index, value, and text of the new selection.
 */

/**
 * Set a callback invoked when selection changes, receiving a SelectionChangedEvent.
 *
 * @param callback Function called with a SelectionChangedEvent describing the change.
 */

/**
 * Set a callback invoked when the dropdown is opened.
 *
 * @param callback Function called when the dropdown opens.
 */

/**
 * Set a callback invoked when the dropdown is closed.
 *
 * @param callback Function called when the dropdown closes.
 */

/**
 * Called when the mouse pointer enters the component's area.
 */

/**
 * Called when the mouse pointer leaves the component's area.
 */

/**
 * Handle a mouse event targeted at this component.
 *
 * @param event Mouse event data.
 * @returns True if the event was consumed, false otherwise.
 */

/**
 * Render the dropdown control.
 *
 * @param renderer Renderer used to draw the component.
 */

/**
 * Handle a keyboard event when this component has focus.
 *
 * @param event Key event data.
 * @returns True if the event was consumed, false otherwise.
 */

/**
 * Update the dropdown state; called periodically with elapsed time.
 *
 * @param deltaTime Time in seconds since the last update.
 */
namespace NomadUI {

    struct DropdownItem {
        std::string text;
        std::function<void()> callback;
        int value = 0;
        bool enabled = true;
        std::string iconPath;  // Optional icon
        
        DropdownItem() = default;
        DropdownItem(const std::string& t, int v = 0) 
            : text(t), value(v), enabled(true) {}
    };

    struct SelectionChangedEvent {
        size_t index;
        int value;
        std::string text;
    };

    class NUIDropdown : public NUIComponent {
    public:
        NUIDropdown();
        virtual ~NUIDropdown();

        // Item management
        void addItem(const std::string& text);
        void addItem(const std::string& text, const std::function<void()>& callback);
        void addItem(const std::string& text, int value);
        void addItem(const std::string& text, int value, const std::function<void()>& callback);
        void addItem(const DropdownItem& item);
        
        void removeItem(size_t index);
        void clear();
        void clearItems();  // Backwards-compatible alias

        // Selection management
        void setSelectedIndex(size_t index);
        void setSelectedIndex(int index);
        void setSelectedByValue(int value);
        
        size_t getSelectedIndex() const;
        std::string getSelectedText() const;
        int getSelectedValue() const;
        std::optional<DropdownItem> getSelectedItem() const;

        // Item access
        int getItemCount() const;
        const std::vector<DropdownItem>& getItems() const;
        std::optional<DropdownItem> getItem(size_t index) const;
        
        // Configuration
        void setPlaceholderText(const std::string& text);
        void setItemHeight(float height);
        void setMaxVisibleItems(size_t count);
        void setSearchEnabled(bool enabled);
        void setAnimationEnabled(bool enabled);
        
        float getItemHeight() const { return itemHeight; }
        size_t getMaxVisibleItems() const { return maxVisibleItems; }
        bool isSearchEnabled() const { return searchEnabled; }

        // Dropdown state
        bool isDropdownOpen() const { return isOpen; }
        void openDropdown();
        void closeDropdown();
        void toggleDropdown();

        // Event callbacks
        void setOnSelectionChanged(const std::function<void(size_t)>& callback);
        void setOnSelectionChanged(const std::function<void(int, int, const std::string&)>& callback);
        void setOnSelectionChangedEx(const std::function<void(const SelectionChangedEvent&)>& callback);
        void setOnDropdownOpened(const std::function<void()>& callback);
        void setOnDropdownClosed(const std::function<void()>& callback);

        // Overridden from NUIComponent
        void onMouseEnter() override;
        void onMouseLeave() override;
        bool onMouseEvent(const NUIMouseEvent& event) override;
        void onRender(NUIRenderer& renderer) override;
        
        // Additional virtual methods (remove override if not in base class)
        virtual bool onKeyEvent(const NUIKeyEvent& event);
        void onUpdate(double deltaTime) override;

    private:
        // Items and selection
        std::vector<DropdownItem> items;
        size_t selectedIndex;
        std::string placeholderText;
        
        // Dropdown state
        bool isOpen;
        NUIPoint lastMousePos;
        std::optional<size_t> hoveredIndex;
        
        // Visual settings
        float itemHeight;
        size_t maxVisibleItems;
        float dropdownAnimationProgress;  // 0.0 = closed, 1.0 = open
        bool animationEnabled;
        
        // Scrolling
        size_t scrollOffset;
        float scrollbarWidth;
        bool needsScrollbar;
        
        // Search functionality
        bool searchEnabled;
        std::string searchBuffer;
        float searchTimeout;
        const float SEARCH_TIMEOUT_DURATION = 1.0f;
        
        // Labels
        std::shared_ptr<NUILabel> currentSelectionLabel;
        
        // Callbacks
        std::function<void(size_t)> onSelectionChangedCallback;
        std::function<void(int, int, const std::string&)> onSelectionChangedFullCallback;
        std::function<void(const SelectionChangedEvent&)> onSelectionChangedExCallback;
        std::function<void()> onDropdownOpenedCallback;
        std::function<void()> onDropdownClosedCallback;

        // Helper methods
        void updateCurrentSelectionLabel();
        void notifySelectionChanged();
        bool isPointInDropdownList(const NUIPoint& point) const;
        bool isPointInMainButton(const NUIPoint& point) const;
        std::optional<size_t> getItemIndexAtPoint(const NUIPoint& point) const;
        
        // Rendering helpers
        void renderMainButton(NUIRenderer& renderer);
        void renderDropdownList(NUIRenderer& renderer);
        void renderItem(NUIRenderer& renderer, size_t index, float y);
        void renderScrollbar(NUIRenderer& renderer);
        
        // Keyboard navigation
        void selectPreviousItem();
        void selectNextItem();
        void selectFirstItem();
        void selectLastItem();
        void handleSearchCharacter(char c);
        
        // Scrolling
        void updateScrollOffset();
        void scrollToItem(size_t index);
        size_t getVisibleItemCount() const;
        float getDropdownHeight() const;
        
        // Animation
        void updateAnimation(float deltaTime);
    };

} // namespace NomadUI

#endif // NUIDROPDOWN_H