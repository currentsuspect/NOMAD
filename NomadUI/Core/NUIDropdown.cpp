// © 2025 Nomad Studios – All Rights Reserved. Licensed for personal & educational use only.
#include "NUIDropdown.h"
#include "../Graphics/NUIRenderer.h"
#include "NUITheme.h"
#include <algorithm>
#include <cmath>

namespace NomadUI {

    /**
     * @brief Constructs a dropdown UI component and initializes its default state.
     *
     * Initializes selection, visual, interaction, scrolling, search, and animation defaults
     * (placeholder text "Select an option..."; item height 30; max visible items 10; scrollbar disabled
     * by default, search and animation enabled). Creates the internal label used to display the
     * current selection and updates it to reflect the initial state.
     */
    NUIDropdown::NUIDropdown() 
        : selectedIndex(0)
        , isOpen(false)
        , lastMousePos(0, 0)
        , hoveredIndex(std::nullopt)
        , itemHeight(30.0f)
        , maxVisibleItems(10)
        , dropdownAnimationProgress(0.0f)
        , animationEnabled(true)
        , scrollOffset(0)
        , scrollbarWidth(8.0f)
        , needsScrollbar(false)
        , searchEnabled(true)
        , searchTimeout(0.0f)
        , placeholderText("Select an option...")
    {
        currentSelectionLabel = std::make_shared<NUILabel>();
        updateCurrentSelectionLabel();
    }

    /**
     * @brief Destroys the dropdown and releases its owned items.
     *
     * Clears the internal list of DropdownItem objects held by this instance.
     */
    NUIDropdown::~NUIDropdown() {
        items.clear();
    }

    // ============================================================
    // Item Management
    /**
     * @brief Adds a new item to the dropdown using the given display text.
     *
     * The new item is assigned the next sequential integer value (current item count)
     * and no selection callback.
     *
     * @param text Display text for the new dropdown item.
     */

    void NUIDropdown::addItem(const std::string& text) {
        addItem(text, static_cast<int>(items.size()), nullptr);
    }

    /**
     * @brief Appends a dropdown item using the next available integer value and associates a selection callback.
     *
     * The new item's value is set to the current item count (i.e., next sequential integer). When the item
     * is selected, the provided callback will be invoked.
     *
     * @param text Display text for the new item.
     * @param callback Function to invoke when the item is selected; may be empty.
     */
    void NUIDropdown::addItem(const std::string& text, const std::function<void()>& callback) {
        addItem(text, static_cast<int>(items.size()), callback);
    }

    /**
     * @brief Adds an item to the dropdown using the provided display text and integer value.
     *
     * @param text Display text for the new item.
     * @param value Integer value associated with the new item.
     */
    void NUIDropdown::addItem(const std::string& text, int value) {
        addItem(text, value, nullptr);
    }

    /**
     * @brief Adds an item to the dropdown with associated text, value, and callback.
     *
     * If this is the first item added, it becomes the current selection and the selection
     * label is updated. The dropdown's scrollbar requirement is recalculated after insertion.
     *
     * @param text Visible label for the new item.
     * @param value Integer value associated with the new item.
     * @param callback Function to invoke when the item is selected (may be empty).
     */
    void NUIDropdown::addItem(const std::string& text, int value, const std::function<void()>& callback) {
        DropdownItem item(text, value);
        item.callback = callback;
        items.push_back(item);
        
        if (items.size() == 1) {
            selectedIndex = 0;
            updateCurrentSelectionLabel();
        }
        
        needsScrollbar = items.size() > maxVisibleItems;
    }

    /**
     * @brief Adds a dropdown item to the end of the list.
     *
     * Appends the provided item to the internal items vector. If this is the first
     * item added, selects it (selectedIndex becomes 0) and updates the current
     * selection label. Recomputes the needsScrollbar flag based on the new item
     * count and maxVisibleItems.
     *
     * @param item DropdownItem to append to the dropdown.
     */
    void NUIDropdown::addItem(const DropdownItem& item) {
        items.push_back(item);
        
        if (items.size() == 1) {
            selectedIndex = 0;
            updateCurrentSelectionLabel();
        }
        
        needsScrollbar = items.size() > maxVisibleItems;
    }

    /**
     * @brief Removes the dropdown item at the given index and updates selection and scrollbar state.
     *
     * If the index is valid, the item at that index is removed. If the removal causes the
     * current selection to move past the end of the list, the selected index is adjusted to
     * the last item or reset to 0 when the list becomes empty; the visible selection label
     * is updated when the selected index changes. The scrollbar requirement is recalculated
     * after removal.
     *
     * @param index Zero-based index of the item to remove.
     */
    void NUIDropdown::removeItem(size_t index) {
        if (index < items.size()) {
            items.erase(items.begin() + index);
            
            if (selectedIndex >= items.size()) {
                selectedIndex = items.empty() ? 0 : items.size() - 1;
                updateCurrentSelectionLabel();
            }
            
            needsScrollbar = items.size() > maxVisibleItems;
        }
    }

    /**
     * @brief Removes all items from the dropdown and resets selection and view state.
     *
     * @details Clears the internal item list, resets the selected index to 0, resets the scroll offset to 0,
     * updates the visible selection label (showing the placeholder when applicable), and disables the scrollbar.
     */
    void NUIDropdown::clear() {
        items.clear();
        selectedIndex = 0;
        scrollOffset = 0;
        updateCurrentSelectionLabel();
        needsScrollbar = false;
    }

    /**
     * @brief Removes all items from the dropdown and resets selection and scrolling state.
     *
     * Clears the item list, resets the selected index and scroll offset, updates the displayed
     * selection label (showing the placeholder if present), and disables the scrollbar.
     */
    void NUIDropdown::clearItems() {
        clear();
    }

    // ============================================================
    // Selection Management
    /**
     * @brief Selects the dropdown item at the given zero-based index.
     *
     * Does nothing if the dropdown has no items, the index is out of range,
     * the item is already selected, or the target item is disabled.
     * When selection changes, updates the visible selection label, invokes
     * registered selection-change callbacks, and calls the item's optional callback.
     *
     * @param index Zero-based index of the item to select.
     */

    void NUIDropdown::setSelectedIndex(size_t index) {
        if (items.empty() || index >= items.size() || index == selectedIndex) {
            return;
        }
        
        if (!items[index].enabled) {
            return;  // Cannot select disabled items
        }
        
        selectedIndex = index;
        updateCurrentSelectionLabel();
        notifySelectionChanged();
        
        if (items[selectedIndex].callback) {
            items[selectedIndex].callback();
        }
    }

    /**
     * @brief Sets the current selection using a signed index.
     *
     * Sets the selected item to the provided index when the index is valid and the item is enabled.
     * Negative indices are ignored. If the index is out of range or refers to a disabled item, no selection change occurs.
     *
     * @param index Signed index of the item to select; negative values are treated as no-ops.
     */
    void NUIDropdown::setSelectedIndex(int index) {
        if (index < 0) return;
        setSelectedIndex(static_cast<size_t>(index));
    }

    /**
     * @brief Selects the first dropdown item whose value equals the provided value.
     *
     * If a matching item is found the selection is updated and selection callbacks are invoked.
     * If no matching item exists the current selection is left unchanged.
     *
     * @param value Integer value to match against item values.
     */
    void NUIDropdown::setSelectedByValue(int value) {
        for (size_t i = 0; i < items.size(); ++i) {
            if (items[i].value == value) {
                setSelectedIndex(i);
                return;
            }
        }
    }

    /**
     * @brief Gets the index of the currently selected item.
     *
     * @return size_t The selected item's index; 0 if no item is selected.
     */
    size_t NUIDropdown::getSelectedIndex() const {
        return selectedIndex;
    }

    /**
     * @brief Gets the text of the currently selected item.
     *
     * @return std::string The selected item's text, or an empty string if no item is selected.
     */
    std::string NUIDropdown::getSelectedText() const {
        if (!items.empty() && selectedIndex < items.size()) {
            return items[selectedIndex].text;
        }
        return "";
    }

    /**
     * @brief Retrieves the value of the currently selected dropdown item.
     *
     * @return int The selected item's `value`, or `0` if there is no selected item.
     */
    int NUIDropdown::getSelectedValue() const {
        if (!items.empty() && selectedIndex < items.size()) {
            return items[selectedIndex].value;
        }
        return 0;
    }

    /**
     * @brief Retrieve the currently selected dropdown item, if present.
     *
     * @return std::optional<DropdownItem> Containing the selected item when the dropdown has items and the selected index is valid; `std::nullopt` otherwise.
     */
    std::optional<DropdownItem> NUIDropdown::getSelectedItem() const {
        if (!items.empty() && selectedIndex < items.size()) {
            return items[selectedIndex];
        }
        return std::nullopt;
    }

    // ============================================================
    // Item Access
    /**
     * @brief Get the number of items in the dropdown.
     *
     * @return int The number of items currently stored in the dropdown.
     */

    int NUIDropdown::getItemCount() const {
        return static_cast<int>(items.size());
    }

    /**
     * @brief Retrieves the current list of dropdown items.
     *
     * Returns a const reference to the internal vector of DropdownItem objects representing
     * the dropdown's items in order.
     *
     * @return const std::vector<DropdownItem>& Reference to the current items vector.
     */
    const std::vector<DropdownItem>& NUIDropdown::getItems() const {
        return items;
    }

    /**
     * @brief Retrieves the dropdown item at the given zero-based index.
     *
     * @param index Zero-based index of the item to retrieve.
     * @return std::optional<DropdownItem> `DropdownItem` when `index` is valid, `std::nullopt` when out of range.
     */
    std::optional<DropdownItem> NUIDropdown::getItem(size_t index) const {
        if (index < items.size()) {
            return items[index];
        }
        return std::nullopt;
    }

    // ============================================================
    // Configuration
    /**
     * @brief Set the placeholder text shown when no item is selected.
     *
     * Updates the internal placeholder string and, if the dropdown contains no items,
     * refreshes the visible selection label to reflect the new placeholder.
     *
     * @param text Placeholder text to display when there is no selection.
     */

    void NUIDropdown::setPlaceholderText(const std::string& text) {
        placeholderText = text;
        if (items.empty()) {
            updateCurrentSelectionLabel();
        }
    }

    /**
     * @brief Sets the height of each item in the dropdown list.
     *
     * The provided height is clamped to a minimum of 20 pixels. After updating the
     * item height, the dropdown recalculates whether a vertical scrollbar is required
     * based on the current item count and the maximum visible items.
     *
     * @param height Desired item height in pixels.
     */
    void NUIDropdown::setItemHeight(float height) {
        itemHeight = std::max(20.0f, height);
        needsScrollbar = items.size() > maxVisibleItems;
    }

    /**
     * @brief Sets the maximum number of items visible in the dropdown list without scrolling.
     *
     * The value is clamped to a minimum of 1. Changing this updates internal scrollbar requirement
     * based on the current number of items.
     *
     * @param count Desired maximum visible item count (will be treated as at least 1).
     */
    void NUIDropdown::setMaxVisibleItems(size_t count) {
        maxVisibleItems = std::max(size_t(1), count);
        needsScrollbar = items.size() > maxVisibleItems;
    }

    /**
     * @brief Enable or disable the dropdown's type-to-search feature.
     *
     * When disabled, any accumulated search query is cleared and the search timeout is reset.
     *
     * @param enabled True to enable type-to-search; false to disable and clear any in-progress search.
     */
    void NUIDropdown::setSearchEnabled(bool enabled) {
        searchEnabled = enabled;
        if (!enabled) {
            searchBuffer.clear();
            searchTimeout = 0.0f;
        }
    }

    /**
     * @brief Enables or disables the dropdown open/close animation.
     *
     * When `enabled` is `false`, the dropdown's animation progress is immediately
     * set to fully open if the dropdown is currently open, or fully closed if not.
     *
     * @param enabled True to enable animations, false to disable and snap progress.
     */
    void NUIDropdown::setAnimationEnabled(bool enabled) {
        animationEnabled = enabled;
        if (!enabled) {
            dropdownAnimationProgress = isOpen ? 1.0f : 0.0f;
        }
    }

    // ============================================================
    // Dropdown State
    /**
     * @brief Opens the dropdown list if it is closed and contains items.
     *
     * Ensures the currently selected item is visible in the list and invokes the
     * registered "dropdown opened" callback if one is set. Does nothing when the
     * dropdown is already open or contains no items.
     */

    void NUIDropdown::openDropdown() {
        if (!isOpen && !items.empty()) {
            isOpen = true;
            scrollToItem(selectedIndex);
            
            if (onDropdownOpenedCallback) {
                onDropdownOpenedCallback();
            }
        }
    }

    /**
     * @brief Closes the dropdown if it is open.
     *
     * When closing, clears hovered item state, resets the type-to-search buffer and timeout,
     * and invokes the registered dropdown-closed callback if one is set.
     */
    void NUIDropdown::closeDropdown() {
        if (isOpen) {
            isOpen = false;
            hoveredIndex = std::nullopt;
            searchBuffer.clear();
            searchTimeout = 0.0f;
            
            if (onDropdownClosedCallback) {
                onDropdownClosedCallback();
            }
        }
    }

    /**
     * @brief Toggles the dropdown between open and closed states.
     *
     * Opens the dropdown if it is currently closed (subject to open conditions), or closes it if it is currently open.
     * Opening and closing will invoke the configured dropdown opened/closed callbacks.
     */
    void NUIDropdown::toggleDropdown() {
        if (isOpen) {
            closeDropdown();
        } else {
            openDropdown();
        }
    }

    // ============================================================
    // Event Callbacks
    /**
     * @brief Registers a callback invoked when the selected item changes.
     *
     * The callback will be called with the newly selected item's index each time the dropdown selection is updated.
     *
     * @param callback Function to call with the new selected index.
     */

    void NUIDropdown::setOnSelectionChanged(const std::function<void(size_t)>& callback) {
        onSelectionChangedCallback = callback;
    }

    /**
     * @brief Registers a callback invoked when the dropdown selection changes.
     *
     * @param callback Function called on selection change; receives three arguments:
     * 1) previous selected index (or -1 if none), 2) new selected index, and
     * 3) the newly selected item's text.
     */
    void NUIDropdown::setOnSelectionChanged(const std::function<void(int, int, const std::string&)>& callback) {
        onSelectionChangedFullCallback = callback;
    }

    /**
     * @brief Registers a callback invoked when the dropdown selection changes.
     *
     * The provided callback is called with a single `SelectionChangedEvent` argument
     * describing the selection transition whenever the active item changes.
     *
     * @param callback Function to invoke on selection changes; receives the `SelectionChangedEvent`.
     */
    void NUIDropdown::setOnSelectionChangedEx(const std::function<void(const SelectionChangedEvent&)>& callback) {
        onSelectionChangedExCallback = callback;
    }

    /**
     * @brief Registers a callback invoked when the dropdown is opened.
     *
     * @param callback Function to call with no arguments when the dropdown is opened. Passing an empty `std::function` clears any previously set handler.
     */
    void NUIDropdown::setOnDropdownOpened(const std::function<void()>& callback) {
        onDropdownOpenedCallback = callback;
    }

    /**
     * @brief Registers a callback to be invoked when the dropdown is closed.
     *
     * @param callback Function to call with no arguments each time the dropdown is closed.
     *                 Passing an empty `std::function` clears any previously registered callback.
     */
    void NUIDropdown::setOnDropdownClosed(const std::function<void()>& callback) {
        onDropdownClosedCallback = callback;
    }

    // ============================================================
    // Component Events
    /**
     * @brief Handle mouse-enter events for the dropdown component.
     *
     * Updates the component's hover state when the mouse enters its bounds.
     */

    void NUIDropdown::onMouseEnter() {
        NUIComponent::onMouseEnter();
    }

    /**
     * @brief Handle the mouse leaving the component.
     *
     * Calls the base class mouse-leave handler and, if the dropdown is open,
     * closes it when the last recorded mouse position lies outside both the
     * main button and the dropdown list.
     */
    void NUIDropdown::onMouseLeave() {
        NUIComponent::onMouseLeave();
        
        // Close dropdown if mouse leaves both button and list
        if (isOpen && !isPointInDropdownList(lastMousePos) && !isPointInMainButton(lastMousePos)) {
            closeDropdown();
        }
    }

    /**
     * @brief Handle mouse input for the dropdown, including clicks, wheel scrolling, and hover tracking.
     *
     * Processes mouse wheel events to scroll the open list, toggles or opens/closes the dropdown
     * on main-button clicks, selects items when visible list items are clicked, updates the hovered
     * item when the list is open, and updates internal mouse position state.
     *
     * @param event The mouse event to process.
     * @return bool `true` if the event was handled and should not propagate further, `false` if it was not handled (allow propagation).
     */
    bool NUIDropdown::onMouseEvent(const NUIMouseEvent& event) {
        lastMousePos = event.position;
        
        // Handle mouse wheel for scrolling
        if (isOpen && event.wheelDelta != 0) {
            if (isPointInDropdownList(event.position)) {
                int newOffset = static_cast<int>(scrollOffset) - event.wheelDelta;
                scrollOffset = std::clamp(newOffset, 0, static_cast<int>(items.size()) - static_cast<int>(maxVisibleItems));
                return true;
            }
        }
        
        // Handle clicks
        if (event.pressed && event.button == NUIMouseButton::Left) {
            if (isPointInMainButton(event.position)) {
                toggleDropdown();
                return true;
            } else if (isOpen && isPointInDropdownList(event.position)) {
                auto clickedIndex = getItemIndexAtPoint(event.position);
                if (clickedIndex.has_value() && items[*clickedIndex].enabled) {
                    setSelectedIndex(*clickedIndex);
                }
                closeDropdown();
                return true;
            } else if (isOpen) {
                // Click outside - close dropdown
                closeDropdown();
                return false;  // Allow event to propagate
            }
        }
        
        // Update hovered item
        if (isOpen) {
            hoveredIndex = getItemIndexAtPoint(event.position);
        }
        
        return NUIComponent::onMouseEvent(event);
    }

    /**
     * @brief Handles keyboard input for dropdown navigation, activation, and search.
     *
     * Processes activation keys (Space/Enter) to open the dropdown or confirm a hovered item, Escape to close,
     * navigation keys (Up/Down/Home/End) to move the hovered/selected item and open the list when needed,
     * and type-to-search characters when search is enabled and the list is open. Updates open state,
     * hovered/selected index, and the search buffer as applicable.
     *
     * @param event Key event to handle.
     * @return true if the event was consumed by the dropdown, false to allow propagation.
     */
    bool NUIDropdown::onKeyEvent(const NUIKeyEvent& event) {
        if (!event.pressed) {
            return NUIComponent::onKeyEvent(event);
        }
        
        // Handle dropdown toggle
        if (event.keyCode == NUIKeyCode::Space || event.keyCode == NUIKeyCode::Enter) {
            if (!isOpen) {
                openDropdown();
                return true;
            } else if (hoveredIndex.has_value()) {
                setSelectedIndex(*hoveredIndex);
                closeDropdown();
                return true;
            }
        }
        
        // Handle escape
        if (event.keyCode == NUIKeyCode::Escape && isOpen) {
            closeDropdown();
            return true;
        }
        
        // Handle arrow keys
        if (event.keyCode == NUIKeyCode::Up) {
            if (!isOpen) {
                openDropdown();
            }
            selectPreviousItem();
            return true;
        }
        
        if (event.keyCode == NUIKeyCode::Down) {
            if (!isOpen) {
                openDropdown();
            }
            selectNextItem();
            return true;
        }
        
        if (event.keyCode == NUIKeyCode::Home) {
            selectFirstItem();
            return true;
        }
        
        if (event.keyCode == NUIKeyCode::End) {
            selectLastItem();
            return true;
        }
        
        // Handle type-to-search
        if (searchEnabled && isOpen && event.character != '\0') {
            handleSearchCharacter(event.character);
            return true;
        }
        
        return NUIComponent::onKeyEvent(event);
    }

    /**
     * @brief Renders the dropdown control.
     *
     * Draws the main button (current selection / placeholder and arrow) and, when the dropdown is visible or animating, renders the dropdown list and its contents.
     *
     * @param renderer Rendering interface used to draw the control.
     */
    void NUIDropdown::onRender(NUIRenderer& renderer) {
        renderMainButton(renderer);
        
        if (dropdownAnimationProgress > 0.0f) {
            renderDropdownList(renderer);
        }
    }

    /**
     * @brief Perform per-frame updates for the dropdown component.
     *
     * Updates base class state, advances the open/close animation when animations are enabled,
     * and decrements the type-to-search timeout; when the timeout reaches zero the search buffer is cleared.
     *
     * @param deltaTime Time elapsed since the last update, in seconds.
     */
    void NUIDropdown::onUpdate(double deltaTime) {
        NUIComponent::onUpdate(deltaTime);
        
        // Update animation
        if (animationEnabled) {
            updateAnimation(static_cast<float>(deltaTime));
        }
        
        // Update search timeout
        if (searchTimeout > 0.0f) {
            searchTimeout -= static_cast<float>(deltaTime);
            if (searchTimeout <= 0.0f) {
                searchBuffer.clear();
                searchTimeout = 0.0f;
            }
        }
    }

    // ============================================================
    // Helper Methods
    /**
     * @brief Synchronizes the visible selection label with the current dropdown state.
     *
     * Updates the text of the internal selection label to the placeholder when the dropdown has
     * no items, to the selected item's text when the selected index is within range, or to an
     * empty string if the selected index is out of range.
     */

    void NUIDropdown::updateCurrentSelectionLabel() {
        std::string text = items.empty() ? placeholderText : 
                          (selectedIndex < items.size() ? items[selectedIndex].text : "");
        currentSelectionLabel->setText(text);
    }

    /**
     * @brief Invokes registered selection-change handlers for the currently selected item.
     *
     * If a valid selection exists, this notifies each configured callback with the selection data:
     * - onSelectionChangedCallback: receives the selected index.
     * - onSelectionChangedFullCallback: receives `(int)index`, `value`, and `text`.
     * - onSelectionChangedExCallback: receives a `SelectionChangedEvent` with `index`, `value`, and `text`.
     */
    void NUIDropdown::notifySelectionChanged() {
        if (items.empty() || selectedIndex >= items.size()) {
            return;
        }
        
        const auto& item = items[selectedIndex];
        
        if (onSelectionChangedCallback) {
            onSelectionChangedCallback(selectedIndex);
        }
        
        if (onSelectionChangedFullCallback) {
            onSelectionChangedFullCallback(static_cast<int>(selectedIndex), item.value, item.text);
        }
        
        if (onSelectionChangedExCallback) {
            SelectionChangedEvent event;
            event.index = selectedIndex;
            event.value = item.value;
            event.text = item.text;
            onSelectionChangedExCallback(event);
        }
    }

    /**
     * @brief Determines whether a point lies within the dropdown's visible list area.
     *
     * The point is tested against the dropdown list rectangle directly below the main control.
     *
     * @param point Point in the same coordinate space as the dropdown (component/screen coordinates).
     * @return bool `true` if the point is inside the dropdown list area and the dropdown is open with items, `false` otherwise.
     */
    bool NUIDropdown::isPointInDropdownList(const NUIPoint& point) const {
        if (!isOpen || items.empty()) return false;

        float dropdownY = getY() + getHeight();
        float dropdownHeight = getDropdownHeight();

        return point.x >= getX() && point.x <= getX() + getWidth() &&
               point.y >= dropdownY && point.y <= dropdownY + dropdownHeight;
    }

    /**
     * @brief Determines whether a point lies within the dropdown's main button area.
     *
     * @param point Point to test, expressed in the same coordinate space as the dropdown.
     * @return `true` if the point is inside the main button bounds, `false` otherwise.
     */
    bool NUIDropdown::isPointInMainButton(const NUIPoint& point) const {
        return point.x >= getX() && point.x <= getX() + getWidth() &&
               point.y >= getY() && point.y <= getY() + getHeight();
    }

    /**
     * @brief Determines which dropdown item (if any) lies under a given point.
     *
     * @param point Point in the same coordinate space used by the component (typically screen or parent local coordinates).
     * @return std::optional<size_t> The zero-based index of the item under the point, or `std::nullopt` if the point is outside the dropdown list or does not correspond to any item.
     */
    std::optional<size_t> NUIDropdown::getItemIndexAtPoint(const NUIPoint& point) const {
        if (!isPointInDropdownList(point)) {
            return std::nullopt;
        }

        float dropdownY = getY() + getHeight();
        float relativeY = point.y - dropdownY;
        size_t index = scrollOffset + static_cast<size_t>(relativeY / itemHeight);
        
        if (index < items.size()) {
            return index;
        }
        
        return std::nullopt;
    }

    // ============================================================
    // Rendering Helpers
    /**
     * @brief Render the dropdown's main (collapsed) button including its background, current selection (or placeholder), and arrow indicator.
     *
     * Renders the button using theme colors and visual states: normal, hovered, and open. Displays the selected item's text or the placeholder when no items exist, and draws the dropdown arrow rotated to indicate the open state. Text is vertically aligned within the button according to the current theme font size.
     */

    void NUIDropdown::renderMainButton(NUIRenderer& renderer) {
        NUIRect rect = {getX(), getY(), getWidth(), getHeight()};
        
        // Determine colors based on state
        NUIColor bgColor = getTheme()->getColor("dropdown.background", NUIColor(0.2f, 0.2f, 0.2f, 1.0f));
        NUIColor borderColor = getTheme()->getColor("dropdown.border", NUIColor(0.4f, 0.4f, 0.4f, 1.0f));
        
        if (isHovered() || isOpen) {
            bgColor = getTheme()->getColor("dropdown.hover", NUIColor(0.25f, 0.25f, 0.25f, 1.0f));
        }
        
        // Draw background and border
        renderer.fillRect(rect, bgColor);
        renderer.strokeRect(rect, 1.0f, borderColor);

        // Draw current selection text with proper baseline alignment
        float textX = getX() + 10.0f;
        float fontSize = getTheme()->getFontSizeNormal();
        
        // Create rect for text area and calculate proper baseline Y
        NUIRect textRect(getX(), getY(), getWidth(), getHeight());
        float textY = textRect.y + textRect.height / 2.0f + fontSize / 6.0f;
        
        NUIColor textColor = items.empty() ? 
            getTheme()->getColor("dropdown.placeholder", NUIColor(0.5f, 0.5f, 0.5f, 1.0f)) :
            getTheme()->getText();
        
        std::string displayText = items.empty() ? placeholderText : items[selectedIndex].text;
        renderer.drawText(displayText, NUIPoint(textX, textY), fontSize, textColor);

        // Draw dropdown arrow
        float arrowSize = 8.0f;
        float arrowX = getX() + getWidth() - arrowSize - 10.0f;
        float arrowY = getY() + (getHeight() - arrowSize) / 2.0f;
        
        float arrowRotation = isOpen ? 180.0f : 0.0f;
        
        NUIPoint p1(arrowX, arrowY);
        NUIPoint p2(arrowX + arrowSize, arrowY);
        NUIPoint p3(arrowX + arrowSize / 2.0f, arrowY + arrowSize);
        
        NUIColor arrowColor = getTheme()->getColor("dropdown.arrow", NUIColor(0.8f, 0.8f, 0.8f, 1.0f));
        renderer.drawLine(p1, p2, 1.5f, arrowColor);
        renderer.drawLine(p2, p3, 1.5f, arrowColor);
        renderer.drawLine(p3, p1, 1.5f, arrowColor);
    }

    /**
     * @brief Renders the dropdown list, its visible items, optional scrollbar, and search indicator.
     *
     * Draws the dropdown background and border below the main control, renders each visible item
     * (taking scrolling and enabled/hover/selection states into account), shows the scrollbar when
     * required, and displays a search indicator text when searching. Honors the animation progress
     * to scale the list's visible height.
     *
     * @param renderer Renderer used for drawing the list, items, scrollbar, and text.
     */
    void NUIDropdown::renderDropdownList(NUIRenderer& renderer) {
        if (items.empty()) return;

        float dropdownY = getY() + getHeight();
        float dropdownHeight = getDropdownHeight();
        
        // Apply animation
        if (animationEnabled && dropdownAnimationProgress < 1.0f) {
            dropdownHeight *= dropdownAnimationProgress;
        }

        // Draw dropdown background
        NUIRect listRect = {getX(), dropdownY, getWidth(), dropdownHeight};
        NUIColor listBg = getTheme()->getColor("dropdown.list.background", NUIColor(0.15f, 0.15f, 0.15f, 1.0f));
        NUIColor listBorder = getTheme()->getColor("dropdown.list.border", NUIColor(0.3f, 0.3f, 0.3f, 1.0f));
        
        renderer.fillRect(listRect, listBg);
        renderer.strokeRect(listRect, 1.0f, listBorder);

        // Draw visible items
        size_t visibleCount = getVisibleItemCount();
        for (size_t i = 0; i < visibleCount && (scrollOffset + i) < items.size(); ++i) {
            size_t itemIndex = scrollOffset + i;
            float itemY = dropdownY + i * itemHeight;
            renderItem(renderer, itemIndex, itemY);
        }

        // Draw scrollbar if needed
        if (needsScrollbar) {
            renderScrollbar(renderer);
        }
        
        // Draw search indicator with proper baseline alignment
        if (searchEnabled && !searchBuffer.empty()) {
            std::string searchText = "Search: " + searchBuffer;
            float fontSize = getTheme()->getFontSizeSmall();
            
            // Create rect for search text and calculate proper baseline Y
            NUIRect searchRect(getX() + 5.0f, dropdownY + dropdownHeight + 5.0f, getWidth() - 10.0f, fontSize + 4.0f);
            float searchY = searchRect.y + searchRect.height / 2.0f + fontSize / 6.0f;
            
            renderer.drawText(searchText, NUIPoint(getX() + 5.0f, searchY), 
                            fontSize, getTheme()->getText());
        }
    }

    /**
     * @brief Renders a single dropdown item at the specified vertical position.
     *
     * Draws the item's background when hovered or selected (respecting the item's enabled state),
     * and renders the item text with baseline-aligned positioning and appropriate color for enabled
     * or disabled states. Respects the scrollbar width when computing the item area.
     *
     * @param renderer Renderer used to draw shapes and text.
     * @param index Index of the item within the dropdown's item list to render.
     * @param y Top Y coordinate (pixels) where the item's rect should be drawn.
     */
    void NUIDropdown::renderItem(NUIRenderer& renderer, size_t index, float y) {
        NUIRect itemRect = {getX(), y, getWidth() - (needsScrollbar ? scrollbarWidth : 0.0f), itemHeight};
        
        const auto& item = items[index];
        
        // Highlight selected or hovered item
        bool isHoveredItem = hoveredIndex.has_value() && *hoveredIndex == index;
        bool isSelectedItem = index == selectedIndex;
        
        if (isHoveredItem && item.enabled) {
            NUIColor hoverColor = getTheme()->getColor("dropdown.item.hover", NUIColor(0.3f, 0.3f, 0.3f, 1.0f));
            renderer.fillRect(itemRect, hoverColor);
        } else if (isSelectedItem) {
            NUIColor selectedColor = getTheme()->getColor("dropdown.item.selected", NUIColor(0.25f, 0.35f, 0.45f, 1.0f));
            renderer.fillRect(itemRect, selectedColor);
        }

        // Draw item text with proper baseline alignment
        NUIColor textColor = item.enabled ? 
            getTheme()->getText() :
            getTheme()->getColor("dropdown.item.disabled", NUIColor(0.4f, 0.4f, 0.4f, 1.0f));
        
        float textX = getX() + 10.0f;
        float fontSize = getTheme()->getFontSizeNormal();
        
        // Create rect for item text area and calculate proper baseline Y
        NUIRect textRect(getX(), y, getWidth(), itemHeight);
        float textY = y + itemHeight / 2.0f + fontSize / 2.0f;
        
        renderer.drawText(item.text, NUIPoint(textX, textY), fontSize, textColor);
    }

    /**
     * @brief Renders the vertical scrollbar for the open dropdown list.
     *
     * Draws a scrollbar whose height reflects the proportion of visible items to total items
     * and whose vertical position represents the current scroll offset. The scrollbar color
     * is taken from the theme key "dropdown.scrollbar" (with a sensible default).
     */
    void NUIDropdown::renderScrollbar(NUIRenderer& renderer) {
        float dropdownY = getY() + getHeight();
        float dropdownHeight = getDropdownHeight();
        
        float scrollbarX = getX() + getWidth() - scrollbarWidth;
        float scrollbarHeight = dropdownHeight * (static_cast<float>(maxVisibleItems) / items.size());
        float scrollbarY = dropdownY + (dropdownHeight - scrollbarHeight) * 
                           (static_cast<float>(scrollOffset) / (items.size() - maxVisibleItems));
        
        NUIRect scrollbarRect = {scrollbarX, scrollbarY, scrollbarWidth, scrollbarHeight};
        NUIColor scrollbarColor = getTheme()->getColor("dropdown.scrollbar", NUIColor(0.5f, 0.5f, 0.5f, 0.8f));
        
        renderer.fillRect(scrollbarRect, scrollbarColor);
    }

    // ============================================================
    // Keyboard Navigation
    /**
     * @brief Selects the previous enabled item in the dropdown, wrapping to the end.
     *
     * Moves the current selection to the nearest enabled item before the current index.
     * If the start of the list is reached, wraps around to the last item. Updates the
     * selected index and scrolls the list to make the new selection visible.
     *
     * If the dropdown has no items or no enabled items, the selection remains unchanged.
     */

    void NUIDropdown::selectPreviousItem() {
        if (items.empty()) return;
        
        size_t newIndex = selectedIndex;
        do {
            if (newIndex == 0) {
                newIndex = items.size() - 1;
            } else {
                newIndex--;
            }
            
            if (items[newIndex].enabled) {
                setSelectedIndex(newIndex);
                scrollToItem(newIndex);
                break;
            }
        } while (newIndex != selectedIndex);
    }

    /**
     * @brief Advances the selection to the next enabled item in the list.
     *
     * If the dropdown has items, this moves the selection forward to the next item whose
     * `enabled` flag is true, wrapping from the end to the start as needed. When a new
     * enabled item is selected, the dropdown scrolls to make that item visible.
     *
     * If no items exist or no other enabled item is found, the selection remains unchanged.
     */
    void NUIDropdown::selectNextItem() {
        if (items.empty()) return;
        
        size_t newIndex = selectedIndex;
        do {
            newIndex = (newIndex + 1) % items.size();
            
            if (items[newIndex].enabled) {
                setSelectedIndex(newIndex);
                scrollToItem(newIndex);
                break;
            }
        } while (newIndex != selectedIndex);
    }

    /**
     * @brief Selects the first enabled item in the dropdown and ensures it is visible.
     *
     * If an enabled item exists, updates the current selection to that item and adjusts the scroll
     * position so the item is visible. If no enabled items are present, no action is taken.
     */
    void NUIDropdown::selectFirstItem() {
        for (size_t i = 0; i < items.size(); ++i) {
            if (items[i].enabled) {
                setSelectedIndex(i);
                scrollToItem(i);
                break;
            }
        }
    }

    /**
     * @brief Selects the last enabled item in the dropdown and makes it visible.
     *
     * If an enabled item is found, updates the current selection to that item and
     * adjusts scrolling so the item is visible. If no enabled items exist, no
     * changes are made.
     */
    void NUIDropdown::selectLastItem() {
        for (int i = static_cast<int>(items.size()) - 1; i >= 0; --i) {
            if (items[i].enabled) {
                setSelectedIndex(i);
                scrollToItem(i);
                break;
            }
        }
    }

    /**
     * @brief Appends a character to the type-ahead search buffer and moves focus to the first matching item.
     *
     * Updates the internal search buffer with the given character and resets the search timeout to
     * SEARCH_TIMEOUT_DURATION. Performs a case-insensitive prefix search over item texts and, if an
     * enabled item matches, sets `hoveredIndex` to that item's index and scrolls the list so the item
     * is visible.
     */
    void NUIDropdown::handleSearchCharacter(char c) {
        searchBuffer += c;
        searchTimeout = SEARCH_TIMEOUT_DURATION;
        
        // Find first item that starts with search buffer
        std::string lowerSearch = searchBuffer;
        std::transform(lowerSearch.begin(), lowerSearch.end(), lowerSearch.begin(), ::tolower);
        
        for (size_t i = 0; i < items.size(); ++i) {
            std::string lowerText = items[i].text;
            std::transform(lowerText.begin(), lowerText.end(), lowerText.begin(), ::tolower);
            
            if (lowerText.find(lowerSearch) == 0 && items[i].enabled) {
                hoveredIndex = i;
                scrollToItem(i);
                break;
            }
        }
    }

    // ============================================================
    // Scrolling
    /**
     * @brief Adjusts the scroll offset so the currently selected item is visible.
     *
     * If the total item count fits within the visible window, the scroll offset
     * is reset to zero. Otherwise the offset is moved as needed so the
     * selected index lies within the visible range [scrollOffset, scrollOffset + maxVisibleItems - 1].
     */

    void NUIDropdown::updateScrollOffset() {
        if (items.size() <= maxVisibleItems) {
            scrollOffset = 0;
            return;
        }
        
        // Ensure selected item is visible
        if (selectedIndex < scrollOffset) {
            scrollOffset = selectedIndex;
        } else if (selectedIndex >= scrollOffset + maxVisibleItems) {
            scrollOffset = selectedIndex - maxVisibleItems + 1;
        }
    }

    /**
     * @brief Adjusts the dropdown's scroll offset so the given item index is visible.
     *
     * Ensures the specified zero-based item index lies within the current visible window.
     * If all items already fit, the scroll offset is reset to 0. If the index is above
     * or below the visible range, the offset is moved just enough to bring that item into view.
     *
     * @param index Zero-based index of the item to scroll into view.
     */
    void NUIDropdown::scrollToItem(size_t index) {
        if (items.size() <= maxVisibleItems) {
            scrollOffset = 0;
            return;
        }
        
        if (index < scrollOffset) {
            scrollOffset = index;
        } else if (index >= scrollOffset + maxVisibleItems) {
            scrollOffset = index - maxVisibleItems + 1;
        }
    }

    /**
     * @brief Computes how many items can be shown in the dropdown list at once.
     *
     * @return size_t The number of visible items, i.e., the smaller of `maxVisibleItems` and the total item count.
     */
    size_t NUIDropdown::getVisibleItemCount() const {
        return std::min(maxVisibleItems, items.size());
    }

    /**
     * @brief Compute the current dropdown list height from visible items.
     *
     * Calculates the total height occupied by the dropdown list by multiplying
     * the number of visible items by the configured item height.
     *
     * @return float Total height of the dropdown list (in pixels).
     */
    float NUIDropdown::getDropdownHeight() const {
        return getVisibleItemCount() * itemHeight;
    }

    // ============================================================
    // Animation
    /**
     * @brief Advances the dropdown open/close animation based on elapsed time.
     *
     * Progresses the internal animation progress toward its target (1.0 when open, 0.0 when closed)
     * using a fixed interpolation speed and snaps to the target when sufficiently close.
     *
     * @param deltaTime Time elapsed, in seconds, since the previous update.
     */

    void NUIDropdown::updateAnimation(float deltaTime) {
        const float animationSpeed = 8.0f;
        
        float target = isOpen ? 1.0f : 0.0f;
        
        if (dropdownAnimationProgress != target) {
            float diff = target - dropdownAnimationProgress;
            dropdownAnimationProgress += diff * animationSpeed * deltaTime;
            
            // Snap to target if very close
            if (std::abs(dropdownAnimationProgress - target) < 0.01f) {
                dropdownAnimationProgress = target;
            }
        }
    }

} // namespace NomadUI