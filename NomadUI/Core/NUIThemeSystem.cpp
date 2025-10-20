#include "NUIThemeSystem.h"
#include <algorithm>
#include <cmath>

namespace NomadUI {

// NUIThemeManager Implementation
NUIThemeManager& NUIThemeManager::getInstance() {
    static NUIThemeManager instance;
    return instance;
}

NUIThemeManager::NUIThemeManager() 
    : currentVariant_(NUIThemeVariant::Dark)
    , activeTheme_("nomad-dark")
    , isTransitioning_(false) {
    initializeDefaultThemes();
}

void NUIThemeManager::initializeDefaultThemes() {
    // Nomad Dark Theme
    NUIThemeProperties nomadDark = NUIThemePresets::createNomadDark();
    themes_["nomad-dark"] = nomadDark;
    
    // Nomad Light Theme
    NUIThemeProperties nomadLight = NUIThemePresets::createNomadLight();
    themes_["nomad-light"] = nomadLight;
    
    // Material Themes
    themes_["material-light"] = NUIThemePresets::createMaterialLight();
    themes_["material-dark"] = NUIThemePresets::createMaterialDark();
    
    // Fluent Themes
    themes_["fluent-light"] = NUIThemePresets::createFluentLight();
    themes_["fluent-dark"] = NUIThemePresets::createFluentDark();
    
    // Cupertino Themes
    themes_["cupertino-light"] = NUIThemePresets::createCupertinoLight();
    themes_["cupertino-dark"] = NUIThemePresets::createCupertinoDark();
    
    // High Contrast Themes
    themes_["high-contrast-light"] = NUIThemePresets::createHighContrastLight();
    themes_["high-contrast-dark"] = NUIThemePresets::createHighContrastDark();
}

void NUIThemeManager::setThemeVariant(NUIThemeVariant variant) {
    currentVariant_ = variant;
    updateSystemTheme();
}

void NUIThemeManager::setCustomTheme(const std::string& name, const NUIThemeProperties& properties) {
    themes_[name] = properties;
}

void NUIThemeManager::setActiveTheme(const std::string& name) {
    if (themes_.find(name) != themes_.end()) {
        activeTheme_ = name;
        if (onThemeChanged_) {
            onThemeChanged_(getCurrentTheme());
        }
    }
}

const NUIThemeProperties& NUIThemeManager::getCurrentTheme() const {
    auto it = themes_.find(activeTheme_);
    if (it != themes_.end()) {
        return it->second;
    }
    return themes_.at("nomad-dark");
}

NUIThemeProperties& NUIThemeManager::getCurrentThemeMutable() {
    return themes_[activeTheme_];
}

void NUIThemeManager::switchTheme(const std::string& name, float durationMs) {
    if (themes_.find(name) == themes_.end()) return;
    
    if (durationMs <= 0.0f) {
        setActiveTheme(name);
        return;
    }
    
    isTransitioning_ = true;
    transitionFromTheme_ = getCurrentTheme();
    transitionToTheme_ = themes_[name];
    
    themeTransitionAnimation_ = std::make_shared<NUIAnimation>();
    themeTransitionAnimation_->setDuration(durationMs);
    themeTransitionAnimation_->setEasing(NUIEasingType::EaseOutCubic);
    themeTransitionAnimation_->setStartValue(0.0f);
    themeTransitionAnimation_->setEndValue(1.0f);
    
    themeTransitionAnimation_->setOnUpdate([this](float progress) {
        // Interpolate between themes
        NUIThemeProperties currentTheme;
        currentTheme.background = NUIColor::lerpHSL(transitionFromTheme_.background, transitionToTheme_.background, progress);
        currentTheme.surface = NUIColor::lerpHSL(transitionFromTheme_.surface, transitionToTheme_.surface, progress);
        currentTheme.primary = NUIColor::lerpHSL(transitionFromTheme_.primary, transitionToTheme_.primary, progress);
        // ... interpolate other properties as needed
        
        if (onThemeChanged_) {
            onThemeChanged_(currentTheme);
        }
    });
    
    themeTransitionAnimation_->setOnComplete([this]() {
        isTransitioning_ = false;
        // Note: activeTheme_ should be set before calling switchTheme
        if (onThemeChanged_) {
            onThemeChanged_(getCurrentTheme());
        }
    });
    
    themeTransitionAnimation_->start();
    NUIAnimationManager::getInstance().addAnimation(themeTransitionAnimation_);
}

void NUIThemeManager::switchThemeVariant(NUIThemeVariant variant, float durationMs) {
    std::string targetTheme;
    switch (variant) {
        case NUIThemeVariant::Light:
            targetTheme = "nomad-light";
            break;
        case NUIThemeVariant::Dark:
            targetTheme = "nomad-dark";
            break;
        case NUIThemeVariant::Auto:
            // TODO: Detect system preference
            targetTheme = "nomad-dark";
            break;
    }
    
    switchTheme(targetTheme, durationMs);
}

void NUIThemeManager::setOnThemeChanged(std::function<void(const NUIThemeProperties&)> callback) {
    onThemeChanged_ = callback;
}

NUIColor NUIThemeManager::getColor(const std::string& colorName) const {
    const auto& theme = getCurrentTheme();
    
    // Core Structure
    if (colorName == "backgroundPrimary") return theme.backgroundPrimary;
    if (colorName == "backgroundSecondary") return theme.backgroundSecondary;
    if (colorName == "surfaceTertiary") return theme.surfaceTertiary;
    if (colorName == "surfaceRaised") return theme.surfaceRaised;
    
    // Legacy compatibility
    if (colorName == "background") return theme.background;
    if (colorName == "surface") return theme.surface;
    if (colorName == "surfaceVariant") return theme.surfaceVariant;
    
    // Accent & Branding
    if (colorName == "primary") return theme.primary;
    if (colorName == "primaryHover") return theme.primaryHover;
    if (colorName == "primaryPressed") return theme.primaryPressed;
    if (colorName == "accent") return theme.primary;
    if (colorName == "secondary") return theme.secondary;
    
    // Functional Colors
    if (colorName == "success") return theme.success;
    if (colorName == "warning") return theme.warning;
    if (colorName == "error") return theme.error;
    if (colorName == "info") return theme.info;
    
    // Text
    if (colorName == "text") return theme.textPrimary;
    if (colorName == "textPrimary") return theme.textPrimary;
    if (colorName == "textSecondary") return theme.textSecondary;
    if (colorName == "textDisabled") return theme.textDisabled;
    if (colorName == "textLink") return theme.textLink;
    if (colorName == "textCritical") return theme.textCritical;
    
    // Borders
    if (colorName == "border") return theme.border;
    if (colorName == "borderSubtle") return theme.borderSubtle;
    if (colorName == "borderActive") return theme.borderActive;
    if (colorName == "divider") return theme.divider;
    
    // Interactive States
    if (colorName == "hover") return theme.hover;
    if (colorName == "pressed") return theme.pressed;
    if (colorName == "focused") return theme.focused;
    if (colorName == "selected") return theme.selected;
    
    // Interactive Elements
    if (colorName == "buttonBgDefault") return theme.buttonBgDefault;
    if (colorName == "buttonBgHover") return theme.buttonBgHover;
    if (colorName == "buttonBgActive") return theme.buttonBgActive;
    if (colorName == "buttonTextDefault") return theme.buttonTextDefault;
    if (colorName == "buttonTextActive") return theme.buttonTextActive;
    
    if (colorName == "toggleDefault") return theme.toggleDefault;
    if (colorName == "toggleHover") return theme.toggleHover;
    if (colorName == "toggleActive") return theme.toggleActive;
    
    if (colorName == "inputBgDefault") return theme.inputBgDefault;
    if (colorName == "inputBgHover") return theme.inputBgHover;
    if (colorName == "inputBorderFocus") return theme.inputBorderFocus;
    
    if (colorName == "sliderTrack") return theme.sliderTrack;
    if (colorName == "sliderHandle") return theme.sliderHandle;
    if (colorName == "sliderHandleHover") return theme.sliderHandleHover;
    if (colorName == "sliderHandlePressed") return theme.sliderHandlePressed;
    
    if (colorName == "highlightGlow") return theme.highlightGlow;
    
    return theme.primary; // Default fallback
}

float NUIThemeManager::getSpacing(const std::string& spacingName) const {
    const auto& theme = getCurrentTheme();
    
    if (spacingName == "xs") return theme.spacingXS;
    if (spacingName == "s") return theme.spacingS;
    if (spacingName == "m") return theme.spacingM;
    if (spacingName == "l") return theme.spacingL;
    if (spacingName == "xl") return theme.spacingXL;
    if (spacingName == "xxl") return theme.spacingXXL;
    
    return theme.spacingM; // Default fallback
}

float NUIThemeManager::getRadius(const std::string& radiusName) const {
    const auto& theme = getCurrentTheme();
    
    if (radiusName == "xs") return theme.radiusXS;
    if (radiusName == "s") return theme.radiusS;
    if (radiusName == "m") return theme.radiusM;
    if (radiusName == "l") return theme.radiusL;
    if (radiusName == "xl") return theme.radiusXL;
    if (radiusName == "xxl") return theme.radiusXXL;
    
    return theme.radiusM; // Default fallback
}

float NUIThemeManager::getFontSize(const std::string& fontSizeName) const {
    const auto& theme = getCurrentTheme();
    
    if (fontSizeName == "xs") return theme.fontSizeXS;
    if (fontSizeName == "s") return theme.fontSizeS;
    if (fontSizeName == "m") return theme.fontSizeM;
    if (fontSizeName == "l") return theme.fontSizeL;
    if (fontSizeName == "xl") return theme.fontSizeXL;
    if (fontSizeName == "xxl") return theme.fontSizeXXL;
    if (fontSizeName == "h1") return theme.fontSizeH1;
    if (fontSizeName == "h2") return theme.fontSizeH2;
    if (fontSizeName == "h3") return theme.fontSizeH3;
    
    return theme.fontSizeM; // Default fallback
}

NUIThemeProperties::Shadow NUIThemeManager::getShadow(const std::string& shadowName) const {
    const auto& theme = getCurrentTheme();
    
    if (shadowName == "xs") return theme.shadowXS;
    if (shadowName == "s") return theme.shadowS;
    if (shadowName == "m") return theme.shadowM;
    if (shadowName == "l") return theme.shadowL;
    if (shadowName == "xl") return theme.shadowXL;
    
    return theme.shadowM; // Default fallback
}

NUIColor NUIThemeManager::getContrastColor(const NUIColor& backgroundColor) const {
    return backgroundColor.textColor();
}

NUIColor NUIThemeManager::getHoverColor(const NUIColor& baseColor) const {
    return baseColor.withLightness(std::min(1.0f, baseColor.toHSL().l + 0.1f));
}

NUIColor NUIThemeManager::getPressedColor(const NUIColor& baseColor) const {
    return baseColor.withLightness(std::max(0.0f, baseColor.toHSL().l - 0.1f));
}

NUIColor NUIThemeManager::getDisabledColor(const NUIColor& baseColor) const {
    return baseColor.withAlpha(0.38f);
}

std::shared_ptr<NUIAnimation> NUIThemeManager::createColorTransition(
    const NUIColor& from, const NUIColor& to, float durationMs) const {
    
    if (durationMs < 0.0f) {
        durationMs = getCurrentTheme().durationNormal;
    }
    
    auto animation = std::make_shared<NUIAnimation>();
    animation->setDuration(durationMs);
    animation->setEasing(getCurrentTheme().easingStandard);
    animation->setStartValue(0.0f);
    animation->setEndValue(1.0f);
    
    return animation;
}

void NUIThemeManager::updateSystemTheme() {
    // TODO: Implement system theme detection
    // For now, just switch to appropriate theme
    if (currentVariant_ == NUIThemeVariant::Light) {
        setActiveTheme("nomad-light");
    } else {
        setActiveTheme("nomad-dark");
    }
}

// NUIThemedComponent Implementation
void NUIThemedComponent::registerForThemeUpdates() {
    if (!isThemeRegistered_) {
        // TODO: Register with theme manager
        isThemeRegistered_ = true;
    }
}

void NUIThemedComponent::unregisterFromThemeUpdates() {
    if (isThemeRegistered_) {
        // TODO: Unregister from theme manager
        isThemeRegistered_ = false;
    }
}

NUIColor NUIThemedComponent::getThemeColor(const std::string& colorName) const {
    return NUIThemeManager::getInstance().getColor(colorName);
}

float NUIThemedComponent::getThemeSpacing(const std::string& spacingName) const {
    return NUIThemeManager::getInstance().getSpacing(spacingName);
}

float NUIThemedComponent::getThemeRadius(const std::string& radiusName) const {
    return NUIThemeManager::getInstance().getRadius(radiusName);
}

float NUIThemedComponent::getThemeFontSize(const std::string& fontSizeName) const {
    return NUIThemeManager::getInstance().getFontSize(fontSizeName);
}

// NUIThemePresets Implementation
NUIThemeProperties NUIThemePresets::createNomadDark() {
    NUIThemeProperties theme;
    
    // 🧱 1. Core Structure - Layered backgrounds
    theme.backgroundPrimary = NUIColor(0.094f, 0.094f, 0.098f, 1.0f);    // #181819 - Deep matte black
    theme.backgroundSecondary = NUIColor(0.118f, 0.118f, 0.122f, 1.0f);  // #1e1e1f - Panels, sidebars
    theme.surfaceTertiary = NUIColor(0.141f, 0.141f, 0.157f, 1.0f);      // #242428 - Dialogs, popups
    theme.surfaceRaised = NUIColor(0.173f, 0.173f, 0.192f, 1.0f);        // #2c2c31 - Cards, containers
    
    // Legacy compatibility
    theme.background = theme.backgroundPrimary;
    theme.surface = theme.backgroundSecondary;
    theme.surfaceVariant = theme.surfaceTertiary;
    
    // 💡 2. Accent & Branding
    theme.primary = NUIColor(0.545f, 0.498f, 1.0f, 1.0f);                // #8B7FFF - Core Nomad purple
    theme.primaryHover = NUIColor(0.655f, 0.620f, 1.0f, 1.0f);           // #A79EFF - Hover variant
    theme.primaryPressed = NUIColor(0.400f, 0.353f, 0.851f, 1.0f);       // #665AD9 - Pressed state
    theme.primaryVariant = theme.primaryPressed;
    
    theme.secondary = theme.backgroundSecondary;
    theme.secondaryVariant = NUIColor(0.5f, 0.5f, 0.6f, 1.0f);
    
    // 🌈 6. Functional Colors (Status Feedback)
    theme.success = NUIColor(0.357f, 0.847f, 0.588f, 1.0f);              // #5BD896 - Green
    theme.warning = NUIColor(1.0f, 0.847f, 0.420f, 1.0f);                // #FFD86B - Amber
    theme.error = NUIColor(1.0f, 0.369f, 0.369f, 1.0f);                  // #FF5E5E - Red
    theme.info = NUIColor(0.420f, 0.796f, 1.0f, 1.0f);                   // #6BCBFF - Cyan-blue
    
    // 🧭 3. Text & Typography
    theme.textPrimary = NUIColor(0.898f, 0.898f, 0.910f, 1.0f);          // #E5E5E8 - Main text
    theme.textSecondary = NUIColor(0.651f, 0.651f, 0.667f, 1.0f);        // #A6A6AA - Subtext
    theme.textDisabled = NUIColor(0.353f, 0.353f, 0.365f, 1.0f);         // #5A5A5D - Inactive
    theme.textLink = theme.primary;                                       // #8B7FFF - Links/actions
    theme.textCritical = theme.error;                                     // #FF5E5E - Errors
    
    // 🪞 5. Borders & Highlights
    theme.borderSubtle = NUIColor(0.173f, 0.173f, 0.184f, 1.0f);         // #2c2c2f - Divider lines
    theme.borderActive = theme.primary;                                   // #8B7FFF - Selected/focused
    theme.border = theme.borderSubtle;
    theme.divider = NUIColor(0.2f, 0.2f, 0.25f, 1.0f);
    theme.outline = NUIColor(0.4f, 0.4f, 0.45f, 1.0f);
    theme.outlineVariant = NUIColor(0.25f, 0.25f, 0.3f, 1.0f);
    
    // 🖱️ 4. Interactive Elements - Buttons
    theme.buttonBgDefault = theme.surfaceTertiary;                        // #242428
    theme.buttonBgHover = NUIColor(0.180f, 0.180f, 0.200f, 1.0f);        // #2e2e33
    theme.buttonBgActive = theme.primary;                                 // #8B7FFF
    theme.buttonTextDefault = theme.textPrimary;                          // #E5E5E8
    theme.buttonTextActive = NUIColor(1.0f, 1.0f, 1.0f, 1.0f);          // #ffffff
    
    // Toggle / Switch
    theme.toggleDefault = NUIColor(0.227f, 0.227f, 0.247f, 1.0f);        // #3a3a3f
    theme.toggleHover = NUIColor(0.290f, 0.290f, 0.314f, 1.0f);          // #4a4a50
    theme.toggleActive = theme.primary;                                   // #8B7FFF
    
    // Input Fields
    theme.inputBgDefault = NUIColor(0.106f, 0.106f, 0.110f, 1.0f);       // #1b1b1c
    theme.inputBgHover = NUIColor(0.122f, 0.122f, 0.125f, 1.0f);         // #1f1f20
    theme.inputBorderFocus = theme.primary;                               // #8B7FFF
    
    // Sliders
    theme.sliderTrack = NUIColor(0.165f, 0.165f, 0.180f, 1.0f);          // #2a2a2e
    theme.sliderHandle = theme.primary;                                   // #8B7FFF
    theme.sliderHandleHover = theme.primaryHover;                         // #A79EFF
    theme.sliderHandlePressed = theme.primaryPressed;                     // #665AD9
    
    // Interactive states (legacy)
    theme.hover = NUIColor(1.0f, 1.0f, 1.0f, 0.08f);
    theme.pressed = NUIColor(1.0f, 1.0f, 1.0f, 0.12f);
    theme.focused = theme.primary.withAlpha(0.2f);
    theme.selected = theme.primary.withAlpha(0.15f);
    theme.disabled = NUIColor(0.5f, 0.5f, 0.5f, 0.38f);
    
    // Highlight glow
    theme.highlightGlow = NUIColor(0.545f, 0.498f, 1.0f, 0.3f);          // rgba(139, 127, 255, 0.3)
    
    // 🪞 5. Shadows
    theme.shadowXS = NUIThemeProperties::Shadow(0, 1, 2, 0, NUIColor::black(), 0.1f);
    theme.shadowS = NUIThemeProperties::Shadow(0, 2, 4, 0, NUIColor::black(), 0.15f);
    theme.shadowM = NUIThemeProperties::Shadow(0, 4, 8, 0, NUIColor::black(), 0.4f);   // Ambient
    theme.shadowL = NUIThemeProperties::Shadow(0, 8, 16, 0, NUIColor::black(), 0.6f);  // Floating UI
    theme.shadowXL = NUIThemeProperties::Shadow(0, 16, 32, 0, NUIColor::black(), 0.6f);
    
    return theme;
}

NUIThemeProperties NUIThemePresets::createNomadLight() {
    NUIThemeProperties theme;
    
    // Colors
    theme.background = NUIColor(0.98f, 0.98f, 0.98f, 1.0f);
    theme.surface = NUIColor(1.0f, 1.0f, 1.0f, 1.0f);
    theme.surfaceVariant = NUIColor(0.95f, 0.95f, 0.95f, 1.0f);
    theme.primary = NUIColor(0.2f, 0.4f, 0.8f, 1.0f);
    theme.primaryVariant = NUIColor(0.1f, 0.3f, 0.7f, 1.0f);
    theme.secondary = NUIColor(0.4f, 0.4f, 0.5f, 1.0f);
    theme.secondaryVariant = NUIColor(0.3f, 0.3f, 0.4f, 1.0f);
    theme.error = NUIColor(0.8f, 0.2f, 0.2f, 1.0f);
    theme.warning = NUIColor(0.9f, 0.6f, 0.1f, 1.0f);
    theme.success = NUIColor(0.2f, 0.7f, 0.3f, 1.0f);
    theme.info = NUIColor(0.1f, 0.6f, 0.8f, 1.0f);
    
    // Text colors
    theme.textPrimary = NUIColor(0.1f, 0.1f, 0.1f, 1.0f);
    theme.textSecondary = NUIColor(0.4f, 0.4f, 0.4f, 1.0f);
    theme.textDisabled = NUIColor(0.6f, 0.6f, 0.6f, 1.0f);
    
    // Interactive states
    theme.hover = NUIColor(0.0f, 0.0f, 0.0f, 0.04f);
    theme.pressed = NUIColor(0.0f, 0.0f, 0.0f, 0.08f);
    theme.focused = theme.primary.withAlpha(0.12f);
    theme.selected = theme.primary.withAlpha(0.08f);
    theme.disabled = NUIColor(0.6f, 0.6f, 0.6f, 0.38f);
    
    // Borders
    theme.border = NUIColor(0.8f, 0.8f, 0.8f, 1.0f);
    theme.divider = NUIColor(0.9f, 0.9f, 0.9f, 1.0f);
    theme.outline = NUIColor(0.7f, 0.7f, 0.7f, 1.0f);
    theme.outlineVariant = NUIColor(0.85f, 0.85f, 0.85f, 1.0f);
    
    // Shadows
    theme.shadowXS = NUIThemeProperties::Shadow(0, 1, 2, 0, NUIColor::black(), 0.05f);
    theme.shadowS = NUIThemeProperties::Shadow(0, 2, 4, 0, NUIColor::black(), 0.08f);
    theme.shadowM = NUIThemeProperties::Shadow(0, 4, 8, 0, NUIColor::black(), 0.12f);
    theme.shadowL = NUIThemeProperties::Shadow(0, 8, 16, 0, NUIColor::black(), 0.15f);
    theme.shadowXL = NUIThemeProperties::Shadow(0, 16, 32, 0, NUIColor::black(), 0.2f);
    
    return theme;
}

// Placeholder implementations for other themes
NUIThemeProperties NUIThemePresets::createMaterialLight() {
    return createNomadLight(); // Simplified for now
}

NUIThemeProperties NUIThemePresets::createMaterialDark() {
    return createNomadDark(); // Simplified for now
}

NUIThemeProperties NUIThemePresets::createFluentLight() {
    return createNomadLight(); // Simplified for now
}

NUIThemeProperties NUIThemePresets::createFluentDark() {
    return createNomadDark(); // Simplified for now
}

NUIThemeProperties NUIThemePresets::createCupertinoLight() {
    return createNomadLight(); // Simplified for now
}

NUIThemeProperties NUIThemePresets::createCupertinoDark() {
    return createNomadDark(); // Simplified for now
}

NUIThemeProperties NUIThemePresets::createHighContrastLight() {
    return createNomadLight(); // Simplified for now
}

NUIThemeProperties NUIThemePresets::createHighContrastDark() {
    return createNomadDark(); // Simplified for now
}

} // namespace NomadUI
