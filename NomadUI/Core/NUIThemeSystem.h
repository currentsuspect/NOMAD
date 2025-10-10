#pragma once

#include "NUITypes.h"
#include "NUIAnimation.h"
#include <unordered_map>
#include <string>
#include <memory>

namespace NomadUI {

// Theme variants
enum class NUIThemeVariant {
    Light,
    Dark,
    Auto  // Follows system preference
};

// Theme properties
struct NUIThemeProperties {
    // Colors
    NUIColor background;
    NUIColor surface;
    NUIColor surfaceVariant;
    NUIColor primary;
    NUIColor primaryVariant;
    NUIColor secondary;
    NUIColor secondaryVariant;
    NUIColor error;
    NUIColor warning;
    NUIColor success;
    NUIColor info;
    NUIColor onBackground;
    NUIColor onSurface;
    NUIColor onPrimary;
    NUIColor onSecondary;
    NUIColor onError;
    NUIColor onWarning;
    NUIColor onSuccess;
    NUIColor onInfo;
    
    // Borders and dividers
    NUIColor border;
    NUIColor divider;
    NUIColor outline;
    NUIColor outlineVariant;
    
    // Interactive states
    NUIColor hover;
    NUIColor pressed;
    NUIColor focused;
    NUIColor selected;
    NUIColor disabled;
    
    // Text colors
    NUIColor textPrimary;
    NUIColor textSecondary;
    NUIColor textDisabled;
    NUIColor textOnPrimary;
    NUIColor textOnSecondary;
    
    // Shadows and overlays
    NUIColor shadow;
    NUIColor overlay;
    NUIColor backdrop;
    
    // Spacing
    float spacingXS = 4.0f;
    float spacingS = 8.0f;
    float spacingM = 16.0f;
    float spacingL = 24.0f;
    float spacingXL = 32.0f;
    float spacingXXL = 48.0f;
    
    // Border radius
    float radiusXS = 2.0f;
    float radiusS = 4.0f;
    float radiusM = 8.0f;
    float radiusL = 12.0f;
    float radiusXL = 16.0f;
    float radiusXXL = 24.0f;
    
    // Typography
    float fontSizeXS = 10.0f;
    float fontSizeS = 12.0f;
    float fontSizeM = 14.0f;
    float fontSizeL = 16.0f;
    float fontSizeXL = 18.0f;
    float fontSizeXXL = 24.0f;
    float fontSizeH1 = 32.0f;
    float fontSizeH2 = 28.0f;
    float fontSizeH3 = 24.0f;
    
    // Line heights
    float lineHeightTight = 1.2f;
    float lineHeightNormal = 1.4f;
    float lineHeightRelaxed = 1.6f;
    
    // Shadows
    struct Shadow {
        float offsetX = 0.0f;
        float offsetY = 0.0f;
        float blurRadius = 0.0f;
        float spreadRadius = 0.0f;
        NUIColor color = NUIColor::black();
        float opacity = 0.0f;
        
        Shadow() = default;
        Shadow(float x, float y, float blur, float spread, const NUIColor& c, float op = 1.0f)
            : offsetX(x), offsetY(y), blurRadius(blur), spreadRadius(spread), color(c), opacity(op) {}
    };
    
    Shadow shadowXS;
    Shadow shadowS;
    Shadow shadowM;
    Shadow shadowL;
    Shadow shadowXL;
    
    // Animation durations
    float durationFast = 150.0f;
    float durationNormal = 250.0f;
    float durationSlow = 350.0f;
    
    // Animation easings
    NUIEasingType easingStandard = NUIEasingType::EaseOutCubic;
    NUIEasingType easingDecelerate = NUIEasingType::EaseOutCubic;
    NUIEasingType easingAccelerate = NUIEasingType::EaseInCubic;
    NUIEasingType easingSharp = NUIEasingType::EaseInOutCubic;
    
    // Z-index layers
    int zIndexBackground = 0;
    int zIndexSurface = 100;
    int zIndexDropdown = 200;
    int zIndexModal = 300;
    int zIndexTooltip = 400;
    int zIndexNotification = 500;
};

// Theme manager
class NUIThemeManager {
public:
    static NUIThemeManager& getInstance();
    
    // Theme management
    void setThemeVariant(NUIThemeVariant variant);
    NUIThemeVariant getThemeVariant() const { return currentVariant_; }
    
    void setCustomTheme(const std::string& name, const NUIThemeProperties& properties);
    void setActiveTheme(const std::string& name);
    std::string getActiveTheme() const { return activeTheme_; }
    
    // Theme access
    const NUIThemeProperties& getCurrentTheme() const;
    NUIThemeProperties& getCurrentThemeMutable();
    
    // Theme switching with animation
    void switchTheme(const std::string& name, float durationMs = 300.0f);
    void switchThemeVariant(NUIThemeVariant variant, float durationMs = 300.0f);
    
    // Theme callbacks
    void setOnThemeChanged(std::function<void(const NUIThemeProperties&)> callback);
    
    // Utility methods
    NUIColor getColor(const std::string& colorName) const;
    float getSpacing(const std::string& spacingName) const;
    float getRadius(const std::string& radiusName) const;
    float getFontSize(const std::string& fontSizeName) const;
    NUIThemeProperties::Shadow getShadow(const std::string& shadowName) const;
    
    // Color utilities
    NUIColor getContrastColor(const NUIColor& backgroundColor) const;
    NUIColor getHoverColor(const NUIColor& baseColor) const;
    NUIColor getPressedColor(const NUIColor& baseColor) const;
    NUIColor getDisabledColor(const NUIColor& baseColor) const;
    
    // Animation utilities
    std::shared_ptr<NUIAnimation> createColorTransition(
        const NUIColor& from, const NUIColor& to, float durationMs = -1.0f) const;
    
private:
    NUIThemeManager();
    void initializeDefaultThemes();
    void updateSystemTheme();
    
    NUIThemeVariant currentVariant_;
    std::string activeTheme_;
    std::unordered_map<std::string, NUIThemeProperties> themes_;
    std::function<void(const NUIThemeProperties&)> onThemeChanged_;
    
    // Animation for theme switching
    std::shared_ptr<NUIAnimation> themeTransitionAnimation_;
    NUIThemeProperties transitionFromTheme_;
    NUIThemeProperties transitionToTheme_;
    bool isTransitioning_;
};

// Theme-aware component base
class NUIThemedComponent {
public:
    virtual ~NUIThemedComponent() = default;
    
    // Theme integration
    virtual void onThemeChanged(const NUIThemeProperties& theme) {}
    virtual void applyTheme(const NUIThemeProperties& theme) {}
    
    // Color helpers
    NUIColor getThemeColor(const std::string& colorName) const;
    float getThemeSpacing(const std::string& spacingName) const;
    float getThemeRadius(const std::string& radiusName) const;
    float getThemeFontSize(const std::string& fontSizeName) const;
    
protected:
    void registerForThemeUpdates();
    void unregisterFromThemeUpdates();
    
private:
    bool isThemeRegistered_ = false;
};

// Predefined theme presets
class NUIThemePresets {
public:
    static NUIThemeProperties createMaterialLight();
    static NUIThemeProperties createMaterialDark();
    static NUIThemeProperties createFluentLight();
    static NUIThemeProperties createFluentDark();
    static NUIThemeProperties createCupertinoLight();
    static NUIThemeProperties createCupertinoDark();
    static NUIThemeProperties createNomadLight();
    static NUIThemeProperties createNomadDark();
    static NUIThemeProperties createHighContrastLight();
    static NUIThemeProperties createHighContrastDark();
};

} // namespace NomadUI
