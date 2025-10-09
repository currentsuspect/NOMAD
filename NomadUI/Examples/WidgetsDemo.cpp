/**
 * Widgets Demo - Nomad UI Framework
 * 
 * A comprehensive demonstration of all NomadUI widgets:
 * - NUILabel - Text display
 * - NUIButton - Interactive buttons
 * - NUISlider - Value selection
 * - NUICheckbox - Toggle states
 * - NUITextInput - Text entry
 * - NUIPanel - Container layouts
 * 
 * This demo showcases:
 * - Widget creation and configuration
 * - Event handling and callbacks
 * - Text rendering capabilities
 * - Theme integration
 * - Layout management
 */

#include "../Core/NUIApp.h"
#include "../Core/NUIComponent.h"
#include "../Core/NUITheme.h"
#include "../Graphics/NUIRenderer.h"
#include "../Widgets/NUIButton.h"
#include "../Widgets/NUILabel.h"
#include "../Widgets/NUISlider.h"
#include "../Widgets/NUICheckbox.h"
#include "../Widgets/NUITextInput.h"
#include "../Widgets/NUIPanel.h"
#include <iostream>
#include <sstream>
#include <iomanip>

using namespace NomadUI;

// ============================================================================
// Demo Application Component
// ============================================================================

class WidgetsDemoPanel : public NUIComponent {
public:
    WidgetsDemoPanel() {
        setupWidgets();
    }
    
    void onRender(NUIRenderer& renderer) override {
        auto theme = getTheme();
        if (!theme) return;
        
        auto bounds = getBounds();
        
        // Draw background
        renderer.fillRect(bounds, theme->getBackground());
        
        // Render all children (widgets)
        NUIComponent::onRender(renderer);
    }
    
    void setupWidgets() {
        // We'll set up widgets after getting theme and bounds
    }
    
    void initializeWidgets() {
        auto theme = getTheme();
        if (!theme) return;
        
        auto bounds = getBounds();
        float padding = 20.0f;
        float col1X = padding;
        float col2X = bounds.width * 0.5f + padding * 0.5f;
        float currentY = padding;
        float widgetHeight = 40.0f;
        float spacing = 15.0f;
        
        // ====================================================================
        // Title
        // ====================================================================
        
        auto title = std::make_shared<NUILabel>("Nomad UI - Widget Gallery");
        title->setBounds(padding, currentY, bounds.width - padding * 2, 50);
        title->setFontSize(theme->getFontSizeTitle());
        title->setTextAlign(NUILabel::TextAlign::Center);
        title->setVerticalAlign(NUILabel::VerticalAlign::Middle);
        title->setTextColor(theme->getPrimary());
        title->setTheme(theme);
        addChild(title);
        
        currentY += 70;
        
        // ====================================================================
        // Left Column - Interactive Widgets
        // ====================================================================
        
        float colWidth = bounds.width * 0.5f - padding * 1.5f;
        
        // Panel for interactive widgets
        auto interactivePanel = std::make_shared<NUIPanel>("Interactive Widgets");
        interactivePanel->setBounds(col1X, currentY, colWidth, 450);
        interactivePanel->setTitleBarEnabled(true);
        interactivePanel->setPadding(15.0f);
        interactivePanel->setShadowEnabled(true);
        interactivePanel->setTheme(theme);
        addChild(interactivePanel);
        
        auto contentBounds = interactivePanel->getContentBounds();
        float panelY = 0;
        
        // Button Demo
        buttonLabel_ = std::make_shared<NUILabel>("Button: Click count = 0");
        buttonLabel_->setBounds(contentBounds.x, contentBounds.y + panelY, contentBounds.width, 30);
        buttonLabel_->setTheme(theme);
        interactivePanel->addChild(buttonLabel_);
        panelY += 35;
        
        auto button = std::make_shared<NUIButton>("Click Me!");
        button->setBounds(contentBounds.x, contentBounds.y + panelY, 150, widgetHeight);
        button->setOnClick([this]() {
            clickCount_++;
            updateButtonLabel();
        });
        button->setTheme(theme);
        interactivePanel->addChild(button);
        panelY += widgetHeight + spacing;
        
        // Slider Demo
        sliderLabel_ = std::make_shared<NUILabel>("Slider: Value = 0.50");
        sliderLabel_->setBounds(contentBounds.x, contentBounds.y + panelY, contentBounds.width, 30);
        sliderLabel_->setTheme(theme);
        interactivePanel->addChild(sliderLabel_);
        panelY += 35;
        
        slider_ = std::make_shared<NUISlider>(0.0f, 1.0f, 0.5f);
        slider_->setBounds(contentBounds.x, contentBounds.y + panelY, contentBounds.width - 20, 30);
        slider_->setOnValueChange([this](float value) {
            updateSliderLabel(value);
        });
        slider_->setTheme(theme);
        interactivePanel->addChild(slider_);
        panelY += 40 + spacing;
        
        // Checkbox Demo
        checkboxLabel_ = std::make_shared<NUILabel>("Checkbox: Unchecked");
        checkboxLabel_->setBounds(contentBounds.x, contentBounds.y + panelY, contentBounds.width, 30);
        checkboxLabel_->setTheme(theme);
        interactivePanel->addChild(checkboxLabel_);
        panelY += 35;
        
        checkbox_ = std::make_shared<NUICheckbox>("Enable feature", false);
        checkbox_->setBounds(contentBounds.x, contentBounds.y + panelY, 200, 30);
        checkbox_->setOnChange([this](bool checked) {
            updateCheckboxLabel(checked);
        });
        checkbox_->setTheme(theme);
        interactivePanel->addChild(checkbox_);
        panelY += 40 + spacing;
        
        // Text Input Demo
        inputLabel_ = std::make_shared<NUILabel>("Text Input: (empty)");
        inputLabel_->setBounds(contentBounds.x, contentBounds.y + panelY, contentBounds.width, 30);
        inputLabel_->setTheme(theme);
        interactivePanel->addChild(inputLabel_);
        panelY += 35;
        
        textInput_ = std::make_shared<NUITextInput>("Enter text here...");
        textInput_->setBounds(contentBounds.x, contentBounds.y + panelY, contentBounds.width - 20, widgetHeight);
        textInput_->setOnTextChange([this](const std::string& text) {
            updateInputLabel(text);
        });
        textInput_->setTheme(theme);
        interactivePanel->addChild(textInput_);
        
        // ====================================================================
        // Right Column - Text & Display Widgets
        // ====================================================================
        
        // Panel for display widgets
        auto displayPanel = std::make_shared<NUIPanel>("Text & Display");
        displayPanel->setBounds(col2X, currentY, colWidth, 450);
        displayPanel->setTitleBarEnabled(true);
        displayPanel->setPadding(15.0f);
        displayPanel->setShadowEnabled(true);
        displayPanel->setTheme(theme);
        addChild(displayPanel);
        
        auto displayContentBounds = displayPanel->getContentBounds();
        float displayY = 0;
        
        // Various text styles
        auto normalLabel = std::make_shared<NUILabel>("Normal Text");
        normalLabel->setBounds(displayContentBounds.x, displayContentBounds.y + displayY, displayContentBounds.width, 30);
        normalLabel->setFontSize(theme->getFontSizeNormal());
        normalLabel->setTheme(theme);
        displayPanel->addChild(normalLabel);
        displayY += 40;
        
        auto largeLabel = std::make_shared<NUILabel>("Large Text");
        largeLabel->setBounds(displayContentBounds.x, displayContentBounds.y + displayY, displayContentBounds.width, 35);
        largeLabel->setFontSize(theme->getFontSizeLarge());
        largeLabel->setTextColor(theme->getPrimary());
        largeLabel->setTheme(theme);
        displayPanel->addChild(largeLabel);
        displayY += 50;
        
        auto smallLabel = std::make_shared<NUILabel>("Small Text");
        smallLabel->setBounds(displayContentBounds.x, displayContentBounds.y + displayY, displayContentBounds.width, 25);
        smallLabel->setFontSize(theme->getFontSizeSmall());
        smallLabel->setTextColor(theme->getTextSecondary());
        smallLabel->setTheme(theme);
        displayPanel->addChild(smallLabel);
        displayY += 35;
        
        auto centeredLabel = std::make_shared<NUILabel>("Centered Text");
        centeredLabel->setBounds(displayContentBounds.x, displayContentBounds.y + displayY, displayContentBounds.width, 30);
        centeredLabel->setTextAlign(NUILabel::TextAlign::Center);
        centeredLabel->setTheme(theme);
        displayPanel->addChild(centeredLabel);
        displayY += 40;
        
        auto shadowLabel = std::make_shared<NUILabel>("Text with Shadow");
        shadowLabel->setBounds(displayContentBounds.x, displayContentBounds.y + displayY, displayContentBounds.width, 30);
        shadowLabel->setShadowEnabled(true);
        shadowLabel->setTheme(theme);
        displayPanel->addChild(shadowLabel);
        displayY += 50;
        
        // Color demonstrations
        auto redLabel = std::make_shared<NUILabel>("Custom Color: Red");
        redLabel->setBounds(displayContentBounds.x, displayContentBounds.y + displayY, displayContentBounds.width, 30);
        redLabel->setTextColor(NUIColor::fromHex(0xff4444));
        redLabel->setTheme(theme);
        displayPanel->addChild(redLabel);
        displayY += 35;
        
        auto greenLabel = std::make_shared<NUILabel>("Custom Color: Green");
        greenLabel->setBounds(displayContentBounds.x, displayContentBounds.y + displayY, displayContentBounds.width, 30);
        greenLabel->setTextColor(NUIColor::fromHex(0x44ff44));
        greenLabel->setTheme(theme);
        displayPanel->addChild(greenLabel);
        displayY += 35;
        
        auto blueLabel = std::make_shared<NUILabel>("Custom Color: Blue");
        blueLabel->setBounds(displayContentBounds.x, displayContentBounds.y + displayY, displayContentBounds.width, 30);
        blueLabel->setTextColor(NUIColor::fromHex(0x4444ff));
        blueLabel->setTheme(theme);
        displayPanel->addChild(blueLabel);
        
        // ====================================================================
        // Bottom Info Panel
        // ====================================================================
        
        currentY += 470;
        
        auto infoPanel = std::make_shared<NUIPanel>();
        infoPanel->setBounds(padding, currentY, bounds.width - padding * 2, 100);
        infoPanel->setPadding(15.0f);
        infoPanel->setBorderEnabled(true);
        infoPanel->setTheme(theme);
        addChild(infoPanel);
        
        auto infoBounds = infoPanel->getContentBounds();
        
        auto infoText = std::make_shared<NUILabel>(
            "Nomad UI Framework - GPU-accelerated, modern UI for C++\n"
            "Features: Widgets, Text Rendering, Themes, Animations"
        );
        infoText->setBounds(infoBounds.x, infoBounds.y, infoBounds.width, 60);
        infoText->setTextAlign(NUILabel::TextAlign::Center);
        infoText->setTextColor(theme->getTextSecondary());
        infoText->setTheme(theme);
        infoPanel->addChild(infoText);
        
        // FPS Counter
        fpsLabel_ = std::make_shared<NUILabel>("FPS: 0");
        fpsLabel_->setBounds(bounds.width - 120, bounds.height - 40, 100, 30);
        fpsLabel_->setTextAlign(NUILabel::TextAlign::Right);
        fpsLabel_->setTextColor(theme->getTextSecondary());
        fpsLabel_->setFontSize(theme->getFontSizeSmall());
        fpsLabel_->setTheme(theme);
        addChild(fpsLabel_);
    }
    
    void setFPS(float fps) {
        if (fpsLabel_) {
            std::ostringstream oss;
            oss << "FPS: " << std::fixed << std::setprecision(0) << fps;
            fpsLabel_->setText(oss.str());
        }
    }
    
private:
    // Widget references for updates
    std::shared_ptr<NUILabel> buttonLabel_;
    std::shared_ptr<NUILabel> sliderLabel_;
    std::shared_ptr<NUILabel> checkboxLabel_;
    std::shared_ptr<NUILabel> inputLabel_;
    std::shared_ptr<NUILabel> fpsLabel_;
    
    std::shared_ptr<NUISlider> slider_;
    std::shared_ptr<NUICheckbox> checkbox_;
    std::shared_ptr<NUITextInput> textInput_;
    
    int clickCount_ = 0;
    
    void updateButtonLabel() {
        if (buttonLabel_) {
            buttonLabel_->setText("Button: Click count = " + std::to_string(clickCount_));
        }
    }
    
    void updateSliderLabel(float value) {
        if (sliderLabel_) {
            std::ostringstream oss;
            oss << "Slider: Value = " << std::fixed << std::setprecision(2) << value;
            sliderLabel_->setText(oss.str());
        }
    }
    
    void updateCheckboxLabel(bool checked) {
        if (checkboxLabel_) {
            checkboxLabel_->setText(checked ? "Checkbox: Checked" : "Checkbox: Unchecked");
        }
    }
    
    void updateInputLabel(const std::string& text) {
        if (inputLabel_) {
            std::string displayText = text.empty() ? "(empty)" : text;
            if (displayText.length() > 30) {
                displayText = displayText.substr(0, 27) + "...";
            }
            inputLabel_->setText("Text Input: " + displayText);
        }
    }
};

// ============================================================================
// Main Application
// ============================================================================

int main(int argc, char** argv) {
    std::cout << "==========================================" << std::endl;
    std::cout << "  Nomad UI - Widgets Demo" << std::endl;
    std::cout << "==========================================" << std::endl;
    std::cout << std::endl;
    
    // Create application
    NUIApp app;
    
    // Initialize
    std::cout << "Initializing..." << std::endl;
    if (!app.initialize(1280, 720, "Nomad UI - Widgets Demo")) {
        std::cerr << "Failed to initialize application!" << std::endl;
        return 1;
    }
    std::cout << "✓ Application initialized" << std::endl;
    
    // Create theme
    auto theme = NUITheme::createDefault();
    std::cout << "✓ Theme loaded" << std::endl;
    
    // Create root component
    auto root = std::make_shared<WidgetsDemoPanel>();
    root->setBounds(0, 0, 1280, 720);
    root->setTheme(theme);
    root->initializeWidgets(); // Initialize widgets after theme is set
    app.setRootComponent(root);
    std::cout << "✓ Widgets created" << std::endl;
    
    // Update callback
    app.onUpdate = [&app, root]() {
        root->setFPS(app.getCurrentFPS());
    };
    
    std::cout << std::endl;
    std::cout << "Widget Gallery:" << std::endl;
    std::cout << "- Button: Click to increment counter" << std::endl;
    std::cout << "- Slider: Drag to adjust value" << std::endl;
    std::cout << "- Checkbox: Click to toggle state" << std::endl;
    std::cout << "- Text Input: Click and type to enter text" << std::endl;
    std::cout << "- Labels: Various text styles and colors" << std::endl;
    std::cout << std::endl;
    std::cout << "Press ESC or close window to quit" << std::endl;
    std::cout << std::endl;
    
    // Run main loop
    app.run();
    
    std::cout << "Shutting down..." << std::endl;
    app.shutdown();
    
    std::cout << "✓ Clean exit" << std::endl;
    return 0;
}
