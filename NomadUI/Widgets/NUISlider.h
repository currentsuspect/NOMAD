#pragma once

#include "../Core/NUIComponent.h"
#include "../Graphics/NUIRenderer.h"
#include <functional>

namespace NomadUI {

/**
 * Slider widget for Nomad UI.
 * 
 * Features:
 * - Horizontal value selection
 * - Min/max range
 * - Smooth dragging
 * - Value change callback
 * - Hover/active states with animation
 */
class NUISlider : public NUIComponent {
public:
    NUISlider();
    NUISlider(float minValue, float maxValue, float initialValue = 0.0f);
    ~NUISlider() override = default;
    
    // ========================================================================
    // Configuration
    // ========================================================================
    
    /**
     * Set the slider value.
     */
    void setValue(float value);
    float getValue() const { return value_; }
    
    /**
     * Set the value range.
     */
    void setRange(float min, float max);
    float getMinValue() const { return minValue_; }
    float getMaxValue() const { return maxValue_; }
    
    /**
     * Set value change callback.
     */
    void setOnValueChange(std::function<void(float)> callback) { onValueChange_ = callback; }
    
    /**
     * Set custom colors (overrides theme).
     */
    void setTrackColor(const NUIColor& color) { trackColor_ = color; useCustomColors_ = true; setDirty(); }
    void setFillColor(const NUIColor& color) { fillColor_ = color; useCustomColors_ = true; setDirty(); }
    void setThumbColor(const NUIColor& color) { thumbColor_ = color; useCustomColors_ = true; setDirty(); }
    
    /**
     * Reset to theme colors.
     */
    void resetColors() { useCustomColors_ = false; setDirty(); }
    
    /**
     * Set thumb size.
     */
    void setThumbRadius(float radius) { thumbRadius_ = radius; setDirty(); }
    float getThumbRadius() const { return thumbRadius_; }
    
    // ========================================================================
    // Component Overrides
    // ========================================================================
    
    void onRender(NUIRenderer& renderer) override;
    void onUpdate(double deltaTime) override;
    bool onMouseEvent(const NUIMouseEvent& event) override;
    
private:
    float value_ = 0.0f;
    float minValue_ = 0.0f;
    float maxValue_ = 1.0f;
    
    bool useCustomColors_ = false;
    NUIColor trackColor_;
    NUIColor fillColor_;
    NUIColor thumbColor_;
    
    bool dragging_ = false;
    float hoverAlpha_ = 0.0f;
    float thumbRadius_ = 8.0f;
    
    std::function<void(float)> onValueChange_;
    
    // Helpers
    float getThumbPosition() const;
    void updateValueFromPosition(float x);
    NUIColor getCurrentTrackColor() const;
    NUIColor getCurrentFillColor() const;
    NUIColor getCurrentThumbColor() const;
};

} // namespace NomadUI
