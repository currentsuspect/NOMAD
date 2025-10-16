#include "../Core/NUIComponent.h"
#include "../Core/NUICustomWindow.h"
#include "../Core/NUIThemeSystem.h"
#include "../Graphics/NUIRenderer.h"
#include "../Graphics/OpenGL/NUIRendererGL.h"
#include "../Platform/Windows/NUIWindowWin32.h"
#include "../External/glad/include/glad/glad.h"
#include <iostream>
#include <memory>

using namespace NomadUI;

class CustomWindowContent : public NUIComponent {
private:
    NUICustomWindow* customWindow_ = nullptr;
    
public:
    void setCustomWindow(NUICustomWindow* window) { customWindow_ = window; }
    
    void onRender(NUIRenderer& renderer) override {
        // Get window dimensions for responsive layout
        NUIRect bounds = getBounds();
        float centerX = bounds.width * 0.5f;
        float centerY = bounds.height * 0.5f;
        
        // Calculate responsive font sizes
        float baseFontSize = std::min(bounds.width, bounds.height) * 0.02f;
        float titleFontSize = std::max(24.0f, baseFontSize * 1.5f);
        float textFontSize = std::max(16.0f, baseFontSize);
        
        // Get theme colors
        auto& themeManager = NUIThemeManager::getInstance();
        NUIColor textColor = themeManager.getColor("text");
        NUIColor accentColor = themeManager.getColor("accent");
        
        // Perfect responsive text centering using measureText
        std::string titleText = "Custom Window Demo";
        NUISize titleSize = renderer.measureText(titleText, titleFontSize);
        NUIPoint textPos(centerX - titleSize.width * 0.5f, centerY - titleSize.height * 2.0f);
        renderer.drawText(titleText, textPos, titleFontSize, textColor);
        
        // Add full-screen mode indicator
        textPos.y += titleSize.height + textFontSize * 0.5f;
        std::string modeText;
        NUIColor modeColor;
        if (customWindow_ && customWindow_->isFullScreen()) {
            modeText = "Full-Screen Mode - Press F11 to restore window";
            modeColor = themeManager.getColor("warning");
        } else {
            modeText = "Window Mode - Title bar with controls visible";
            modeColor = themeManager.getColor("accent");
        }
        NUISize modeSize = renderer.measureText(modeText, textFontSize * 0.8f);
        textPos.x = centerX - modeSize.width * 0.5f;
        renderer.drawText(modeText, textPos, textFontSize * 0.8f, modeColor);
        
        // Draw instructions - perfectly centered with dynamic spacing
        textPos.y += titleSize.height + textFontSize * 0.8f; // Dynamic spacing based on font size
        std::string instruction1 = "This is a custom window with title bar";
        NUISize instr1Size = renderer.measureText(instruction1, textFontSize);
        textPos.x = centerX - instr1Size.width * 0.5f;
        renderer.drawText(instruction1, textPos, textFontSize, textColor);
        
        textPos.y += textFontSize * 1.3f; // Dynamic line spacing
        std::string instruction2 = "Press F11 to toggle full screen";
        NUISize instr2Size = renderer.measureText(instruction2, textFontSize);
        textPos.x = centerX - instr2Size.width * 0.5f;
        renderer.drawText(instruction2, textPos, textFontSize, textColor);
        
        textPos.y += textFontSize * 1.3f;
        std::string instruction3 = "Drag the title bar to move the window";
        NUISize instr3Size = renderer.measureText(instruction3, textFontSize);
        textPos.x = centerX - instr3Size.width * 0.5f;
        renderer.drawText(instruction3, textPos, textFontSize, textColor);
        
        textPos.y += textFontSize * 1.3f;
        std::string instruction4 = "Use the window controls (minimize, maximize, close)";
        NUISize instr4Size = renderer.measureText(instruction4, textFontSize);
        textPos.x = centerX - instr4Size.width * 0.5f;
        renderer.drawText(instruction4, textPos, textFontSize, textColor);
        
        // Draw window size info - perfectly centered with dynamic spacing
        textPos.y += textFontSize * 2.5f; // More space before debug info
        std::string sizeInfo = "Window Size: " + std::to_string((int)bounds.width) + " x " + std::to_string((int)bounds.height);
        NUISize sizeInfoSize = renderer.measureText(sizeInfo, textFontSize * 0.9f);
        textPos.x = centerX - sizeInfoSize.width * 0.5f;
        renderer.drawText(sizeInfo, textPos, textFontSize * 0.9f, accentColor);
        
        // Draw some decorative elements - perfectly responsive positioning
        float rectWidth = std::min(bounds.width * 0.08f, 80.0f); // 8% of width, max 80px
        float rectHeight = rectWidth * 0.75f; // 75% of width for nice proportions
        float rectSpacing = rectWidth * 0.6f; // 60% of width for spacing
        float rectY = centerY + textFontSize * 3.0f; // Dynamic Y position based on font size
        
        NUIRect rect1(centerX - rectWidth - rectSpacing, rectY, rectWidth, rectHeight);
        NUIRect rect2(centerX - rectWidth * 0.5f, rectY, rectWidth, rectHeight);
        NUIRect rect3(centerX + rectSpacing, rectY, rectWidth, rectHeight);
        
        NUIColor rectColor = themeManager.getColor("surface");
        renderer.fillRect(rect1, rectColor);
        renderer.fillRect(rect2, rectColor);
        renderer.fillRect(rect3, rectColor);
    }
};

class CustomWindowDemo : public NUIComponent {
private:
    std::shared_ptr<NUICustomWindow> customWindow_;
    std::shared_ptr<NUIComponent> contentArea_;

public:
    CustomWindowDemo() {
        // Apply Nomad theme
        auto& themeManager = NUIThemeManager::getInstance();
        themeManager.setActiveTheme("nomad-dark");
        
        // Create custom window
        customWindow_ = std::make_shared<NUICustomWindow>();
        customWindow_->setTitle("Nomad Custom Window Demo");
        
        // Create content area with responsive content
        contentArea_ = std::make_shared<CustomWindowContent>();
        customWindow_->setContent(contentArea_.get());
        
        addChild(customWindow_);
    }
    
    void onRender(NUIRenderer& renderer) override {
        // Draw background
        auto& themeManager = NUIThemeManager::getInstance();
        NUIColor bgColor = themeManager.getColor("background");
        renderer.fillRect(getBounds(), bgColor);
        
        // Render children
        renderChildren(renderer);
    }
    
    void onResize(int width, int height) override {
        // Update custom window size
        customWindow_->setBounds(NUIRect(0, 0, width, height));
        NUIComponent::onResize(width, height);
    }
    
    bool onKeyEvent(const NUIKeyEvent& event) override {
        if (event.pressed) {
            switch (event.keyCode) {
                case NUIKeyCode::F11:
                    std::cout << "F11 pressed - toggling full screen" << std::endl;
                    customWindow_->toggleFullScreen();
                    return true;
                case NUIKeyCode::Escape:
                    if (customWindow_->isFullScreen()) {
                        std::cout << "Escape pressed - exiting full screen" << std::endl;
                        customWindow_->exitFullScreen();
                        return true;
                    }
                    break;
            }
        }
        return NUIComponent::onKeyEvent(event);
    }
};

// Main demo function
int main() {
    std::cout << "Nomad Custom Window Demo" << std::endl;
    std::cout << "========================" << std::endl;
    std::cout << "Controls:" << std::endl;
    std::cout << "  F11 - Toggle full screen" << std::endl;
    std::cout << "  Escape - Exit full screen" << std::endl;
    std::cout << "  Custom title bar with window controls" << std::endl;
    std::cout << std::endl;
    
    // Create window
    NUIWindowWin32 window;
    if (!window.create("Nomad Custom Window Demo", 1000, 700)) {
        std::cerr << "Failed to create window" << std::endl;
        return -1;
    }
    
    // Create renderer
    NUIRendererGL renderer;
    if (!renderer.initialize(800, 600)) {
        std::cerr << "Failed to initialize renderer" << std::endl;
        return -1;
    }
    
    // Create demo component
    CustomWindowDemo demo;
    demo.setBounds(NUIRect(0, 0, 1000, 700));
    
    // Set up window
    window.setRootComponent(&demo);
    window.setRenderer(&renderer);
    window.show();
    
    // Connect custom window to platform window
    auto children = demo.getChildren();
    if (!children.empty()) {
        auto customWindow = std::dynamic_pointer_cast<NUICustomWindow>(children[0]);
        if (customWindow) {
            customWindow->setWindowHandle(&window);
            
            // Connect custom window to content for full-screen state checking
            auto contentChildren = customWindow->getChildren();
            if (contentChildren.size() > 1) { // title bar + content
                auto content = std::dynamic_pointer_cast<CustomWindowContent>(contentChildren[1]);
                if (content) {
                    content->setCustomWindow(customWindow.get());
                }
            }
        }
    }
    
    // Main loop
    while (window.processEvents()) {
        // Clear screen
        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        
        // Render
        renderer.beginFrame();
        demo.onRender(renderer);
        renderer.endFrame();
        
        // Swap buffers
        window.swapBuffers();
    }
    
    std::cout << "Demo completed successfully!" << std::endl;
    return 0;
}
