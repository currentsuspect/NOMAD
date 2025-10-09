#pragma once

#include "../Core/NUIComponent.h"
#include "../Graphics/NUIRenderer.h"
#include <string>
#include <functional>

namespace NomadUI {

/**
 * Checkbox widget for Nomad UI.
 * 
 * Features:
 * - Checked/unchecked states
 * - Optional label text
 * - Smooth check animation
 * - Change callback
 * - Hover/active states
 */
class NUICheckbox : public NUIComponent {
public:
    NUICheckbox();
    NUICheckbox(const std::string& label, bool checked = false);
    ~NUICheckbox() override = default;
    
    // ========================================================================
    // Configuration
    // ========================================================================
    
    /**
     * Set checked state.
     */
    void setChecked(bool checked);
    bool isChecked() const { return checked_; }
    
    /**
     * Toggle checked state.
     */
    void toggle();
    
    /**
     * Set label text.
     */
    void setLabel(const std::string& label);
    std::string getLabel() const { return label_; }
    
    /**
     * Set change callback.
     */
    void setOnChange(std::function<void(bool)> callback) { onChange_ = callback; }
    
    /**
     * Set custom colors (overrides theme).
     */
    void setBoxColor(const NUIColor& color) { boxColor_ = color; useCustomColors_ = true; setDirty(); }
    void setCheckColor(const NUIColor& color) { checkColor_ = color; useCustomColors_ = true; setDirty(); }
    void setLabelColor(const NUIColor& color) { labelColor_ = color; useCustomColors_ = true; setDirty(); }
    
    /**
     * Reset to theme colors.
     */
    void resetColors() { useCustomColors_ = false; setDirty(); }
    
    /**
     * Set box size.
     */
    void setBoxSize(float size) { boxSize_ = size; setDirty(); }
    float getBoxSize() const { return boxSize_; }
    
    // ========================================================================
    // Component Overrides
    // ========================================================================
    
    void onRender(NUIRenderer& renderer) override;
    void onUpdate(double deltaTime) override;
    bool onMouseEvent(const NUIMouseEvent& event) override;
    
private:
    bool checked_ = false;
    std::string label_;
    float boxSize_ = 20.0f;
    
    bool useCustomColors_ = false;
    NUIColor boxColor_;
    NUIColor checkColor_;
    NUIColor labelColor_;
    
    bool pressed_ = false;
    float hoverAlpha_ = 0.0f;
    float checkAlpha_ = 0.0f; // Animated check state
    
    std::function<void(bool)> onChange_;
    
    NUIRect getBoxBounds() const;
    NUIColor getCurrentBoxColor() const;
    NUIColor getCurrentCheckColor() const;
    NUIColor getCurrentLabelColor() const;
};

} // namespace NomadUI
