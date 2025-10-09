#pragma once

#include "../Core/NUIComponent.h"
#include "../Graphics/NUIRenderer.h"
#include <string>

namespace NomadUI {

/**
 * Panel widget for Nomad UI.
 * 
 * Features:
 * - Container for other widgets
 * - Optional title bar
 * - Customizable background
 * - Border and shadow effects
 * - Padding support
 */
class NUIPanel : public NUIComponent {
public:
    NUIPanel();
    NUIPanel(const std::string& title);
    ~NUIPanel() override = default;
    
    // ========================================================================
    // Configuration
    // ========================================================================
    
    /**
     * Set panel title.
     */
    void setTitle(const std::string& title);
    std::string getTitle() const { return title_; }
    
    /**
     * Enable/disable title bar.
     */
    void setTitleBarEnabled(bool enabled) { titleBarEnabled_ = enabled; setDirty(); }
    bool isTitleBarEnabled() const { return titleBarEnabled_; }
    
    /**
     * Set custom colors (overrides theme).
     */
    void setBackgroundColor(const NUIColor& color) { backgroundColor_ = color; useCustomColors_ = true; setDirty(); }
    void setTitleBarColor(const NUIColor& color) { titleBarColor_ = color; useCustomColors_ = true; setDirty(); }
    void setTitleColor(const NUIColor& color) { titleColor_ = color; useCustomColors_ = true; setDirty(); }
    
    /**
     * Reset to theme colors.
     */
    void resetColors() { useCustomColors_ = false; setDirty(); }
    
    /**
     * Set padding for child components.
     */
    void setPadding(float padding) { padding_ = padding; setDirty(); }
    float getPadding() const { return padding_; }
    
    /**
     * Enable/disable border.
     */
    void setBorderEnabled(bool enabled) { borderEnabled_ = enabled; setDirty(); }
    bool isBorderEnabled() const { return borderEnabled_; }
    
    /**
     * Enable/disable shadow effect.
     */
    void setShadowEnabled(bool enabled) { shadowEnabled_ = enabled; setDirty(); }
    bool isShadowEnabled() const { return shadowEnabled_; }
    
    /**
     * Get the content area bounds (excluding title bar and padding).
     */
    NUIRect getContentBounds() const;
    
    // ========================================================================
    // Component Overrides
    // ========================================================================
    
    void onRender(NUIRenderer& renderer) override;
    
private:
    std::string title_;
    bool titleBarEnabled_ = false;
    float titleBarHeight_ = 30.0f;
    float padding_ = 10.0f;
    bool borderEnabled_ = true;
    bool shadowEnabled_ = false;
    
    bool useCustomColors_ = false;
    NUIColor backgroundColor_;
    NUIColor titleBarColor_;
    NUIColor titleColor_;
    
    NUIColor getCurrentBackgroundColor() const;
    NUIColor getCurrentTitleBarColor() const;
    NUIColor getCurrentTitleColor() const;
};

} // namespace NomadUI
