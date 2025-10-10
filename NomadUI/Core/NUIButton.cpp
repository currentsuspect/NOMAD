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
        pressScale = 1.03f; // Slight scale up on hover
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
        // Enhanced rounded rectangle with modern gradient effect
        float cornerRadius = theme.radiusM;
        
        // Subtle shadow for depth
        NUIRect shadowRect = scaledBounds;
        shadowRect.x += 1;
        shadowRect.y += 2;
        renderer.fillRoundedRect(shadowRect, cornerRadius, theme.shadowS.color.withAlpha(theme.shadowS.opacity * opacity));
        
        // Hover glow effect with smooth transition
        if (state_ == State::Hovered)
        {
            NUIRect glowRect = scaledBounds;
            glowRect.x -= 3;
            glowRect.y -= 3;
            glowRect.width += 6;
            glowRect.height += 6;
            renderer.fillRoundedRect(glowRect, cornerRadius + 3, bgColor.withAlpha(0.2f * opacity));
        }
        
        // Modern gradient effect using HSL color space
        NUIColor baseColor = bgColor.withAlpha(opacity);
        NUIColor topColor = baseColor.withLightness(std::min(1.0f, baseColor.toHSL().l + 0.08f));
        NUIColor bottomColor = baseColor.withLightness(std::max(0.0f, baseColor.toHSL().l - 0.05f));
        
        // Draw gradient background with smooth transitions
        for (int i = 0; i < 4; ++i)
        {
            float factor = static_cast<float>(i) / 3.0f;
            NUIColor gradientColor = NUIColor::lerpHSL(topColor, bottomColor, factor);
            NUIRect gradientRect = scaledBounds;
            gradientRect.y += i * 1.5f;
            gradientRect.height -= i * 1.5f;
            renderer.fillRoundedRect(gradientRect, cornerRadius, gradientColor);
        }
        
        // Inner highlight for modern look
        NUIRect highlightRect = scaledBounds;
        highlightRect.x += 1;
        highlightRect.y += 1;
        highlightRect.width -= 2;
        highlightRect.height = scaledBounds.height * 0.4f;
        renderer.fillRoundedRect(highlightRect, cornerRadius - 1, 
            topColor.withAlpha(0.3f * opacity));
        
        // Enhanced border with state-based styling
        float borderWidth = (state_ == State::Hovered) ? 2.0f : 1.5f;
        NUIColor borderColor = (state_ == State::Hovered) ? 
            bgColor.lightened(0.3f).withAlpha(opacity) : 
            bgColor.lightened(0.15f).withAlpha(opacity);
        renderer.strokeRoundedRect(scaledBounds, cornerRadius, borderWidth, borderColor);
        break;
    }
        case Style::Secondary:
        {
            // Enhanced outlined button with better contrast
            float cornerRadius = 6.0f;
            
            // Hover glow effect
            if (state_ == State::Hovered)
            {
                NUIRect glowRect = scaledBounds;
                glowRect.x -= 1;
                glowRect.y -= 1;
                glowRect.width += 2;
                glowRect.height += 2;
                renderer.fillRoundedRect(glowRect, cornerRadius + 1, textColor.withAlpha(0.2f));
            }
            
            // Subtle background
            renderer.fillRoundedRect(scaledBounds, cornerRadius, bgColor.withAlpha(0.1f));
            
            // Enhanced border with hover effect
            float borderWidth = (state_ == State::Hovered) ? 2.5f : 2.0f;
            renderer.strokeRoundedRect(scaledBounds, cornerRadius, borderWidth, textColor);
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
            // Enhanced circular background with shadow
            auto center = scaledBounds.getCentre();
            float radius = std::min(scaledBounds.getWidth(), scaledBounds.getHeight()) * 0.4f;
            
            // Hover glow effect
            if (state_ == State::Hovered)
            {
                renderer.fillCircle(center, radius + 3, bgColor.withAlpha(0.3f));
            }
            
            // Shadow
            NUIPoint shadowCenter = center;
            shadowCenter.x += 1;
            shadowCenter.y += 1;
            renderer.fillCircle(shadowCenter, radius, NUIColor(0, 0, 0, 0.3f));
            
            // Main circle with gradient effect
            NUIColor topColor = bgColor.lightened(0.2f);
            NUIColor bottomColor = bgColor.darkened(0.1f);
            renderer.fillCircle(center, radius, topColor);
            renderer.fillCircle(center, radius * 0.8f, bottomColor);
            
            // Border
            renderer.strokeCircle(center, radius, 1.0f, bgColor.lightened(0.4f));
            break;
        }
    }

    // Draw text
    if (!text_.empty() && style_ != Style::Icon)
    {
        // TODO: Get font from theme
        // renderer.setFont(NUITheme::getDefaultFont());
        renderer.drawTextCentered(text_, scaledBounds, 24.0f, textColor);
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
    state_ = State::Normal;
    repaint();
}

bool NUIButton::onMouseEvent(const NUIMouseEvent& event)
{
    if (!enabled_) return false;
    
    // Check if event is within bounds
    if (!containsPoint(event.position)) {
        if (isPressed_) {
            isPressed_ = false;
            state_ = isHovered() ? State::Hovered : State::Normal;
            repaint();
        }
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
        
        state_ = isHovered() ? State::Hovered : State::Normal;
        repaint();
        return true;
    }
    
    return false;
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
    state_ = State::Hovered; // Will be updated by mouse enter/leave

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
    else
    {
        state_ = State::Normal;
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
