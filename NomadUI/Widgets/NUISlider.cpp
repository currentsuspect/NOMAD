#include "NUISlider.h"
#include "../Core/NUITheme.h"
#include <algorithm>

namespace NomadUI {

NUISlider::NUISlider() : NUISlider(0.0f, 1.0f, 0.5f) {
}

NUISlider::NUISlider(float minValue, float maxValue, float initialValue)
    : minValue_(minValue), maxValue_(maxValue), value_(initialValue) {
}

void NUISlider::setValue(float value) {
    float newValue = std::clamp(value, minValue_, maxValue_);
    if (value_ != newValue) {
        value_ = newValue;
        setDirty();
        
        if (onValueChange_) {
            onValueChange_(value_);
        }
    }
}

void NUISlider::setRange(float min, float max) {
    minValue_ = min;
    maxValue_ = max;
    setValue(value_); // Clamp to new range
}

void NUISlider::onRender(NUIRenderer& renderer) {
    auto theme = getTheme();
    if (!theme) {
        return;
    }
    
    auto bounds = getBounds();
    float trackHeight = 4.0f;
    float trackY = bounds.y + bounds.height * 0.5f - trackHeight * 0.5f;
    
    // Draw glow effect when hovered or dragging
    if (hoverAlpha_ > 0.01f || dragging_) {
        float glowIntensity = dragging_ ? 0.6f : hoverAlpha_ * 0.3f;
        NUIRect thumbBounds = {
            bounds.x + getThumbPosition() - thumbRadius_,
            bounds.y + bounds.height * 0.5f - thumbRadius_,
            thumbRadius_ * 2.0f,
            thumbRadius_ * 2.0f
        };
        renderer.drawGlow(thumbBounds, thumbRadius_ * 2.0f, glowIntensity, theme->getPrimary());
    }
    
    // Draw track background
    NUIRect trackRect = {bounds.x, trackY, bounds.width, trackHeight};
    renderer.fillRoundedRect(trackRect, trackHeight * 0.5f, getCurrentTrackColor());
    
    // Draw filled portion
    float fillWidth = getThumbPosition();
    if (fillWidth > 0) {
        NUIRect fillRect = {bounds.x, trackY, fillWidth, trackHeight};
        renderer.fillRoundedRect(fillRect, trackHeight * 0.5f, getCurrentFillColor());
    }
    
    // Draw thumb
    float thumbX = bounds.x + getThumbPosition();
    float thumbY = bounds.y + bounds.height * 0.5f;
    
    renderer.fillCircle(
        NUIPoint{thumbX, thumbY},
        thumbRadius_ + (dragging_ ? 2.0f : 0.0f),
        getCurrentThumbColor()
    );
    
    // Draw thumb border
    renderer.strokeCircle(
        NUIPoint{thumbX, thumbY},
        thumbRadius_ + (dragging_ ? 2.0f : 0.0f),
        2.0f,
        theme->getPrimary()
    );
    
    // Render children
    NUIComponent::onRender(renderer);
}

void NUISlider::onUpdate(double deltaTime) {
    // Animate hover effect
    float targetAlpha = isHovered() ? 1.0f : 0.0f;
    float speed = 8.0f;
    
    if (hoverAlpha_ < targetAlpha) {
        hoverAlpha_ += speed * deltaTime;
        if (hoverAlpha_ > targetAlpha) {
            hoverAlpha_ = targetAlpha;
        }
        setDirty();
    } else if (hoverAlpha_ > targetAlpha) {
        hoverAlpha_ -= speed * deltaTime;
        if (hoverAlpha_ < targetAlpha) {
            hoverAlpha_ = targetAlpha;
        }
        setDirty();
    }
    
    NUIComponent::onUpdate(deltaTime);
}

bool NUISlider::onMouseEvent(const NUIMouseEvent& event) {
    if (!isEnabled()) {
        return false;
    }
    
    auto bounds = getBounds();
    
    // Start dragging
    if (event.pressed && event.button == NUIMouseButton::Left) {
        if (containsPoint(event.position)) {
            dragging_ = true;
            updateValueFromPosition(event.position.x);
            setDirty();
            return true;
        }
    }
    
    // Continue dragging
    if (dragging_ && event.button == NUIMouseButton::Left) {
        updateValueFromPosition(event.position.x);
        setDirty();
        return true;
    }
    
    // Stop dragging
    if (event.released && event.button == NUIMouseButton::Left) {
        if (dragging_) {
            dragging_ = false;
            setDirty();
            return true;
        }
    }
    
    return false;
}

float NUISlider::getThumbPosition() const {
    auto bounds = getBounds();
    float range = maxValue_ - minValue_;
    if (range == 0.0f) return 0.0f;
    
    float normalizedValue = (value_ - minValue_) / range;
    return normalizedValue * bounds.width;
}

void NUISlider::updateValueFromPosition(float x) {
    auto bounds = getBounds();
    float normalizedX = std::clamp((x - bounds.x) / bounds.width, 0.0f, 1.0f);
    float newValue = minValue_ + normalizedX * (maxValue_ - minValue_);
    setValue(newValue);
}

NUIColor NUISlider::getCurrentTrackColor() const {
    auto theme = getTheme();
    
    if (useCustomColors_) {
        return trackColor_;
    }
    
    if (!theme) {
        return NUIColor::fromHex(0x333333);
    }
    
    return theme->getSurface().withBrightness(0.8f);
}

NUIColor NUISlider::getCurrentFillColor() const {
    auto theme = getTheme();
    
    if (useCustomColors_) {
        return fillColor_;
    }
    
    if (!theme) {
        return NUIColor::fromHex(0xa855f7);
    }
    
    return theme->getPrimary();
}

NUIColor NUISlider::getCurrentThumbColor() const {
    auto theme = getTheme();
    
    if (useCustomColors_) {
        return thumbColor_;
    }
    
    if (!theme) {
        return NUIColor::white();
    }
    
    if (dragging_) {
        return theme->getPrimary().withBrightness(1.2f);
    }
    
    if (isHovered()) {
        return theme->getPrimary().withBrightness(1.1f);
    }
    
    return theme->getPrimary();
}

} // namespace NomadUI
