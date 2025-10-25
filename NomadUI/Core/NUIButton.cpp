#include "NUIButton.h"
#include "NUITheme.h"
#include "NUIAnimation.h"
#include "NUIThemeSystem.h"
#include "Graphics/NUIRenderer.h"
#include <algorithm>
#include <cmath>

namespace NomadUI {

NUIButton::NUIButton(const std::string& text)
    : text_(text)
{
    setSize(100, 32); // Default size
}

void NUIButton::onRender(NUIRenderer& renderer)
{
    auto bounds = getBounds();
    if (bounds.isEmpty()) return;

    // Get theme colors
    const auto& theme = NUIThemeManager::getInstance().getCurrentTheme();
    
    // Calculate smooth micro-motion animation with easing
    float pressScale = 1.0f;
    float opacity = 1.0f;
    
    if (state_ == State::Pressed)
    {
        pressScale = 0.96f; // Button "presses in" smoothly
        opacity = 0.9f;
    }
    else if (state_ == State::Hovered)
    {
        pressScale = 1.0f; // No scaling on hover for smoother appearance
        opacity = 1.0f;
    }
    else if (state_ == State::Disabled)
    {
        pressScale = 1.0f;
        opacity = 0.6f;
    }
    
    // Apply scaling to bounds with smooth interpolation
    NUIRect scaledBounds = bounds;
    if (pressScale != 1.0f)
    {
        float scaleOffset = (1.0f - pressScale) * 0.5f;
        scaledBounds.x += bounds.width * scaleOffset;
        scaledBounds.y += bounds.height * scaleOffset;
        scaledBounds.width *= pressScale;
        scaledBounds.height *= pressScale;
    }

    // Get current colors based on state
    NUIColor bgColor = backgroundColor_;
    NUIColor textColor = textColor_;

    // Update colors based on state
    switch (state_)
    {
        case State::Hovered:
            if (enabled_)
                bgColor = hoverColor_;
            break;
        case State::Pressed:
            if (enabled_)
                bgColor = pressedColor_;
            break;
        case State::Disabled:
            bgColor = backgroundColor_.withAlpha(0.5f);
            textColor = textColor_.withAlpha(0.5f);
            break;
        case State::Normal:
        default:
            break;
    }

    // Apply toggle state
    if (toggleable_ && toggled_)
    {
        bgColor = pressedColor_;
    }

    // Draw background with enhanced graphics
    switch (style_)
    {
    case Style::Primary:
    {
        // Simplified primary button with smooth hover effect
        float cornerRadius = theme.radiusM;

        // Subtle shadow for depth (only when not hovered for cleaner look)
        if (state_ != State::Hovered)
        {
            NUIRect shadowRect = scaledBounds;
            shadowRect.x += 1;
            shadowRect.y += 1;
            renderer.fillRoundedRect(shadowRect, cornerRadius, theme.shadowS.color.withAlpha(theme.shadowS.opacity * opacity));
        }

        // Simple hover glow effect - similar to secondary button
        if (state_ == State::Hovered)
        {
            NUIRect glowRect = scaledBounds;
            glowRect.x -= 1;
            glowRect.y -= 1;
            glowRect.width += 2;
            glowRect.height += 2;
            renderer.fillRoundedRect(glowRect, cornerRadius + 1, bgColor.withAlpha(0.3f * opacity));
        }

        // Simple flat background with subtle gradient
        NUIColor baseColor = bgColor.withAlpha(opacity);
        NUIColor topColor = baseColor.withLightness(std::min(1.0f, baseColor.toHSL().l + 0.05f));
        NUIColor bottomColor = baseColor.withLightness(std::max(0.0f, baseColor.toHSL().l - 0.05f));

        // Simple two-tone gradient instead of complex multi-layer
        renderer.fillRoundedRect(scaledBounds, cornerRadius, topColor);
        NUIRect bottomRect = scaledBounds;
        bottomRect.y += scaledBounds.height * 0.6f;
        bottomRect.height = scaledBounds.height * 0.4f;
        renderer.fillRoundedRect(bottomRect, cornerRadius, bottomColor);

        // Simple border with smooth hover effect
        float borderWidth = (state_ == State::Hovered) ? 2.0f : 1.5f;
        NUIColor borderColor = (state_ == State::Hovered) ?
            bgColor.lightened(0.2f).withAlpha(opacity) :
            bgColor.lightened(0.1f).withAlpha(opacity);
        renderer.strokeRoundedRect(scaledBounds, cornerRadius, borderWidth, borderColor);
        break;
    }
        case Style::Secondary:
        {
            // Enhanced outlined button with better contrast
            float cornerRadius = 6.0f;

            // Calculate squeeze effect when pressed (squeeze from both sides)
            NUIRect renderBounds = scaledBounds;
            if (isPressed_)
            {
                // Squeeze from both sides - reduce width by 2px total (1px each side)
                renderBounds.x += 1.0f;
                renderBounds.width -= 2.0f;
                cornerRadius = 5.0f; // Slightly smaller radius when pressed
            }

            // Hover glow effect
            if (state_ == State::Hovered)
            {
                NUIRect glowRect = renderBounds;
                glowRect.x -= 1;
                glowRect.y -= 1;
                glowRect.width += 2;
                glowRect.height += 2;
                renderer.fillRoundedRect(glowRect, cornerRadius + 1, textColor.withAlpha(0.2f));
            }

            // Subtle background
            renderer.fillRoundedRect(renderBounds, cornerRadius, bgColor.withAlpha(0.1f));

            // Enhanced border with hover effect
            float borderWidth = (state_ == State::Hovered) ? 2.5f : 2.0f;
            renderer.strokeRoundedRect(renderBounds, cornerRadius, borderWidth, textColor);
            break;
        }
        case Style::Text:
        {
            // Subtle background on hover with glow
            if (state_ == State::Hovered)
            {
                float cornerRadius = 4.0f;
                NUIRect glowRect = scaledBounds;
                glowRect.x -= 1;
                glowRect.y -= 1;
                glowRect.width += 2;
                glowRect.height += 2;
                renderer.fillRoundedRect(glowRect, cornerRadius + 1, bgColor.withAlpha(0.15f));
                renderer.fillRoundedRect(scaledBounds, cornerRadius, bgColor.withAlpha(0.1f));
            }
            break;
        }
        case Style::Icon:
        {
            // Enhanced circular background using rounded rect for better anti-aliasing
            auto center = scaledBounds.getCentre();
            float radius = std::min(scaledBounds.getWidth(), scaledBounds.getHeight()) * 0.4f;
            float cornerRadius = radius; // Use full radius for perfect circle
            
            // Hover glow effect
            if (state_ == State::Hovered)
            {
                NUIRect glowRect = scaledBounds;
                glowRect.x -= 3;
                glowRect.y -= 3;
                glowRect.width += 6;
                glowRect.height += 6;
                renderer.fillRoundedRect(glowRect, cornerRadius + 3, bgColor.withAlpha(0.3f * opacity));
            }
            
            // Shadow
            NUIRect shadowRect = scaledBounds;
            shadowRect.x += 1;
            shadowRect.y += 1;
            renderer.fillRoundedRect(shadowRect, cornerRadius, NUIColor(0, 0, 0, 0.3f * opacity));
            
            // Main circle with gradient effect using rounded rect
            NUIColor topColor = bgColor.lightened(0.2f).withAlpha(opacity);
            NUIColor bottomColor = bgColor.darkened(0.1f).withAlpha(opacity);
            renderer.fillRoundedRect(scaledBounds, cornerRadius, topColor);
            
            // Inner circle for gradient effect
            NUIRect innerRect = scaledBounds;
            float innerPadding = radius * 0.2f;
            innerRect.x += innerPadding;
            innerRect.y += innerPadding;
            innerRect.width -= innerPadding * 2;
            innerRect.height -= innerPadding * 2;
            renderer.fillRoundedRect(innerRect, cornerRadius - innerPadding, bottomColor);
            
            // Smooth border with anti-aliasing
            renderer.strokeRoundedRect(scaledBounds, cornerRadius, 1.5f, bgColor.lightened(0.4f).withAlpha(opacity));
            break;
        }
    }

    // Draw text
    if (!text_.empty() && style_ != Style::Icon)
    {
        // TODO: Get font from theme
        // renderer.setFont(NUITheme::getDefaultFont());
        // Use smaller font size for compact buttons
        float fontSize = std::min(14.0f, scaledBounds.height * 0.5f);

        // Adjust text position for better vertical centering
        NUIRect textRect = scaledBounds;
        if (style_ == Style::Secondary) {
            // Apply squeeze effect to text bounds when pressed
            if (isPressed_)
            {
                textRect.x += 1.0f;
                textRect.width -= 2.0f;
            }
            // Drop Secondary button text down by 2px for better alignment
            textRect.y += 4.0f;
        }

        // Calculate text color with hover preview effect for Secondary buttons
        NUIColor finalTextColor = textColor;
        if (style_ == Style::Secondary && state_ == State::Hovered && !isPressed_)
        {
            // Preview active state colors on hover for M and S buttons
            if (text_ == "M") {
                // Show red preview for mute button (like when muted)
                finalTextColor = NUIColor(1.0f, 0.27f, 0.4f, 1.0f); // Error red color
            } else if (text_ == "S") {
                // Show lime preview for solo button (like when soloed)
                finalTextColor = NUIColor(0.8f, 1.0f, 0.2f, 1.0f); // Accent lime color
            }
        }

        renderer.drawTextCentered(text_, textRect, fontSize, finalTextColor);
    }
}

void NUIButton::onMouseEnter()
{
    if (enabled_)
    {
        state_ = State::Hovered;
        repaint();
    }
}

void NUIButton::onMouseLeave()
{
    // Always reset to normal state when mouse leaves, regardless of press state
    state_ = State::Normal;
    repaint();
}

bool NUIButton::onMouseEvent(const NUIMouseEvent& event)
{
    if (!enabled_) return false;

    // Let base class handle hover detection and child events first
    bool handled = NUIComponent::onMouseEvent(event);

    // If this event was not within bounds, return early
    if (!containsPoint(event.position)) {
        return false;
    }

    // Handle mouse down
    if (event.pressed && event.button == NUIMouseButton::Left)
    {
        isPressed_ = true;
        state_ = State::Pressed;
        repaint();
        return true;
    }

    // Handle mouse up
    if (event.released && event.button == NUIMouseButton::Left)
    {
        bool wasPressed = isPressed_;
        isPressed_ = false;

        if (containsPoint(event.position))
        {
            if (toggleable_)
            {
                toggled_ = !toggled_;
                triggerToggle();
            }
            else
            {
                triggerClick();
            }
        }

        // Don't manually set state - let hover system handle it
        repaint();
        return true;
    }

    return handled;
}

void NUIButton::onMouseDown(int x, int y, int button)
{
    if (!enabled_) return;

    state_ = State::Pressed;
    isPressed_ = true;
    repaint();
}

void NUIButton::onMouseUp(int x, int y, int button)
{
    if (!enabled_) return;

    bool wasPressed = isPressed_;
    isPressed_ = false;

    // Let the hover system handle the state - don't set it manually here
    // The base class hover tracking will set it to Hovered if mouse is still over button

    if (wasPressed)
    {
        if (toggleable_)
        {
            toggled_ = !toggled_;
            triggerToggle();
        }
        else
        {
            triggerClick();
        }
    }

    repaint();
}

void NUIButton::setText(const std::string& text)
{
    text_ = text;
    repaint();
}

void NUIButton::setStyle(Style style)
{
    style_ = style;
    repaint();
}

void NUIButton::setEnabled(bool enabled)
{
    enabled_ = enabled;
    updateState();
    repaint();
}

void NUIButton::setToggleable(bool toggleable)
{
    toggleable_ = toggleable;
}

void NUIButton::setToggled(bool toggled)
{
    if (toggleable_)
    {
        toggled_ = toggled;
        repaint();
    }
}

void NUIButton::setOnClick(std::function<void()> callback)
{
    onClickCallback_ = callback;
}

void NUIButton::setOnToggle(std::function<void(bool)> callback)
{
    onToggleCallback_ = callback;
}

void NUIButton::setBackgroundColor(const NUIColor& color)
{
    backgroundColor_ = color;
    repaint();
}

void NUIButton::setTextColor(const NUIColor& color)
{
    textColor_ = color;
    repaint();
}

void NUIButton::setHoverColor(const NUIColor& color)
{
    hoverColor_ = color;
    repaint();
}

void NUIButton::setPressedColor(const NUIColor& color)
{
    pressedColor_ = color;
    repaint();
}

void NUIButton::updateState()
{
    if (!enabled_)
    {
        state_ = State::Disabled;
    }
    else if (state_ == State::Disabled)
    {
        // If coming out of disabled state, go to normal or hovered
        state_ = isHovered() ? State::Hovered : State::Normal;
    }
    else if (state_ != State::Pressed)
    {
        // Preserve current state unless it's pressed (pressed state is managed by mouse events)
        state_ = isHovered() ? State::Hovered : State::Normal;
    }
}

void NUIButton::triggerClick()
{
    if (onClickCallback_)
    {
        onClickCallback_();
    }
}

void NUIButton::triggerToggle()
{
    if (onToggleCallback_)
    {
        onToggleCallback_(toggled_);
    }
}

} // namespace NomadUI
