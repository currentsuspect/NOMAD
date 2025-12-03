// Â© 2025 Nomad Studios â€” All Rights Reserved. Licensed for personal & educational use only.
#pragma once

#include "NUIComponent.h"
#include "NUITypes.h"
#include <functional>
#include <string>

namespace NomadUI {

/**
 * NUIButton - A customizable button component
 * Replaces juce::TextButton with NomadUI styling and theming
 */
class NUIButton : public NUIComponent
{
public:
    // Button states
    enum class State
    {
        Normal,
        Hovered,
        Pressed,
        Disabled
    };

    /**
 * Create a button with optional label text.
 * @param text Initial label text for the button.
 */
/**
 * Render the button using the provided renderer.
 * @param renderer Renderer used to draw the button.
 */
/**
 * Handle an incoming mouse event and update button state.
 * @param event Mouse event to handle.
 * @returns `true` if the event was handled by the button, `false` otherwise.
 */
/**
 * Called when the mouse cursor enters the button bounds.
 */
/**
 * Called when the mouse cursor leaves the button bounds.
 */
/**
 * Handle mouse press at the given coordinates.
 * @param x X coordinate of the press relative to the button.
 * @param y Y coordinate of the press relative to the button.
 * @param button Mouse button identifier.
 */
/**
 * Handle mouse release at the given coordinates.
 * @param x X coordinate of the release relative to the button.
 * @param y Y coordinate of the release relative to the button.
 * @param button Mouse button identifier.
 */
/**
 * Set the button's label text.
 * @param text New label text.
 */
/**
 * Set the visual style of the button.
 * @param style Visual style to apply.
 */
/**
 * Enable or disable the button.
 * @param enabled `true` to enable the button, `false` to disable it.
 */
/**
 * Enable or disable toggle behavior.
 * @param toggleable `true` to make the button toggleable, `false` otherwise.
 */
/**
 * Set the current toggled state for a toggleable button.
 * @param toggled New toggled state.
 */
/**
 * Set a callback invoked when the button is clicked.
 * @param callback Function called with no arguments on click.
 */
/**
 * Set a callback invoked when the button toggles.
 * @param callback Function called with the new toggled state (`true` or `false`).
 */
/**
 * Set the button background color.
 * @param color Background color to use.
 */
/**
 * Set the button text color.
 * @param color Text color to use.
 */
/**
 * Set the color used when the button is hovered.
 * @param color Hover color to use.
 */
/**
 * Set the color used when the button is pressed.
 * @param color Pressed color to use.
 */
/**
 * Set the font size used to render the button label; `0` uses the theme default.
 * @param size Font size in points, or `0` to use the theme default.
 */
/**
 * Get the currently configured font size.
 * @returns The font size in points, or `0` if the theme default is used.
 */
    enum class Style
    {
        Primary,    // Main action button
        Secondary,  // Secondary action button
        Text,       // Text-only button
        Icon        // Icon-only button
    };

    NUIButton(const std::string& text = "");
    ~NUIButton() override = default;

    // Component interface
    void onRender(NUIRenderer& renderer) override;
    bool onMouseEvent(const NUIMouseEvent& event) override;
    void onMouseEnter() override;
    void onMouseLeave() override;
    void onMouseDown(int x, int y, int button);
    void onMouseUp(int x, int y, int button);

    // Button properties
    void setText(const std::string& text);
    const std::string& getText() const { return text_; }

    void setStyle(Style style);
    Style getStyle() const { return style_; }

    void setEnabled(bool enabled);
    bool isEnabled() const { return enabled_; }

    void setToggleable(bool toggleable);
    bool isToggleable() const { return toggleable_; }

    void setToggled(bool toggled);
    bool isToggled() const { return toggled_; }

    // State access
    bool isHovered() const { return state_ == State::Hovered; }
    bool isPressed() const { return isPressed_; }

    // Event callbacks
    void setOnClick(std::function<void()> callback);
    void setOnToggle(std::function<void(bool)> callback);

    // Theme colors
    void setBackgroundColor(const NUIColor& color);
    void setTextColor(const NUIColor& color);
    void setHoverColor(const NUIColor& color);
    void setPressedColor(const NUIColor& color);
    /**
 * Set the button's font size in points; pass 0 to use the theme default.
 * @param size Font size in points; `0` selects the theme's default font size.
 */
void setFontSize(float size) { fontSize_ = size; }
    float getFontSize() const { return fontSize_; }

private:
    void updateState();
    void triggerClick();
    void triggerToggle();

    std::string text_;
    Style style_ = Style::Primary;
    State state_ = State::Normal;
    bool enabled_ = true;
    bool toggleable_ = false;
    bool toggled_ = false;
    bool isPressed_ = false;

    // Colors
    NUIColor backgroundColor_ = NUIColor::fromHex(0xff9933ff); // Purple theme
    NUIColor textColor_ = NUIColor::fromHex(0xffffffff);
    NUIColor hoverColor_ = NUIColor::fromHex(0xffaa44ff);
    NUIColor pressedColor_ = NUIColor::fromHex(0xff8822ee);
    float fontSize_ = 0.0f; // 0 means use theme default

    // Callbacks
    std::function<void()> onClickCallback_;
    std::function<void(bool)> onToggleCallback_;
};

} // namespace NomadUI