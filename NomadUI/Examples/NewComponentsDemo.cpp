#include <glad/glad.h>
#include <iostream>
#include <memory>
#include <chrono>
#include <thread>

#include "Core/NUIComponent.h"
#include "Core/NUIButton.h"
#include "Core/NUILabel.h"
#include "Core/NUISlider.h"
#include "Core/NUICheckbox.h"
#include "Core/NUITextInput.h"
#include "Core/NUIProgressBar.h"
#include "Core/NUIScrollbar.h"
#include "Core/NUIContextMenu.h"
#include "Core/NUITheme.h"
#include "Graphics/OpenGL/NUIRendererGL.h"
#include "Platform/Windows/NUIWindowWin32.h"

using namespace NomadUI;

class NewComponentsDemo : public NUIComponent
{
private:
    std::shared_ptr<NUIContextMenu> contextMenu_;

public:
    NewComponentsDemo()
    {
        // Create a theme
        auto theme = std::make_shared<NUITheme>();
        setTheme(theme);
        
        // Create components
        createComponents();
    }
    
    void createComponents()
    {
        // Button
        auto button = std::make_shared<NUIButton>("Test Button");
        button->setBounds(NUIRect(20, 20, 120, 30));
        button->setOnClick([]() {
            std::cout << "Button clicked!" << std::endl;
        });
        addChild(button);
        
        // Label
        auto label = std::make_shared<NUILabel>("New Components Demo");
        label->setBounds(NUIRect(20, 60, 200, 25));
        label->setTextColor(NUIColor::fromHex(0xffa855f7));
        addChild(label);
        
        // Slider
        auto slider = std::make_shared<NUISlider>("Volume");
        slider->setBounds(NUIRect(20, 100, 200, 20));
        slider->setRange(0.0, 100.0);
        slider->setValue(50.0);
        slider->setOnValueChange([](double value) {
            std::cout << "Slider value: " << value << std::endl;
        });
        addChild(slider);
        
        // Checkbox
        auto checkbox = std::make_shared<NUICheckbox>("Enable Feature");
        checkbox->setBounds(NUIRect(20, 140, 150, 20));
        checkbox->setOnCheckedChange([](bool checked) {
            std::cout << "Checkbox: " << (checked ? "checked" : "unchecked") << std::endl;
        });
        addChild(checkbox);
        
        // Text Input
        auto textInput = std::make_shared<NUITextInput>("Enter text here...");
        textInput->setBounds(NUIRect(20, 180, 200, 30));
        textInput->setPlaceholderText("Type something...");
        textInput->setOnTextChange([](const std::string& text) {
            std::cout << "Text changed: " << text << std::endl;
        });
        addChild(textInput);
        
        // Progress Bar
        auto progressBar = std::make_shared<NUIProgressBar>();
        progressBar->setBounds(NUIRect(20, 220, 200, 20));
        progressBar->setMinValue(0.0);
        progressBar->setMaxValue(100.0);
        progressBar->setProgress(75.0);
        progressBar->setAnimated(true);
        addChild(progressBar);
        
        // Vertical Scrollbar
        auto vScrollbar = std::make_shared<NUIScrollbar>(NUIScrollbar::Orientation::Vertical);
        vScrollbar->setBounds(NUIRect(250, 20, 16, 200)); // 16px width
        vScrollbar->setRangeLimit(0.0, 100.0);
        vScrollbar->setCurrentRange(0.0, 20.0);
        vScrollbar->setSingleStepSize(5.0);  // Step size for arrow buttons
        vScrollbar->setPageStepSize(20.0);   // Step size for track clicks
        addChild(vScrollbar);
        
        // Horizontal Scrollbar
        auto hScrollbar = std::make_shared<NUIScrollbar>(NUIScrollbar::Orientation::Horizontal);
        hScrollbar->setBounds(NUIRect(20, 250, 200, 16)); // 16px height
        hScrollbar->setRangeLimit(0.0, 100.0);
        hScrollbar->setCurrentRange(0.0, 20.0);
        hScrollbar->setSingleStepSize(5.0);  // Step size for arrow buttons
        hScrollbar->setPageStepSize(20.0);   // Step size for track clicks
        addChild(hScrollbar);
        
        // Context Menu
        contextMenu_ = std::make_shared<NUIContextMenu>();
        contextMenu_->addItem("Cut", []() { std::cout << "Cut selected" << std::endl; });
        contextMenu_->addItem("Copy", []() { std::cout << "Copy selected" << std::endl; });
        contextMenu_->addItem("Paste", []() { std::cout << "Paste selected" << std::endl; });
        contextMenu_->addSeparator();
        contextMenu_->addItem("Settings", []() { std::cout << "Settings selected" << std::endl; });
        
        // Add shortcuts to some items
        auto cutItem = contextMenu_->getItem(0);
        if (cutItem) cutItem->setShortcut("Ctrl+X");
        
        auto copyItem = contextMenu_->getItem(1);
        if (copyItem) copyItem->setShortcut("Ctrl+C");
        
        auto pasteItem = contextMenu_->getItem(2);
        if (pasteItem) pasteItem->setShortcut("Ctrl+V");
        
        addChild(contextMenu_);
    }
    
    bool onMouseEvent(const NUIMouseEvent& event) override
    {
        // Handle right-click to show context menu
        if (event.pressed && event.button == NUIMouseButton::Right)
        {
            if (contextMenu_)
            {
                contextMenu_->showAt(event.position);
                return true;
            }
        }
        
        // Handle left-click to hide context menu if clicking outside
        if (event.pressed && event.button == NUIMouseButton::Left)
        {
            if (contextMenu_ && contextMenu_->isVisible())
            {
                // Check if click is outside context menu bounds
                NUIRect contextBounds = contextMenu_->getBounds();
                if (!contextBounds.contains(event.position))
                {
                    contextMenu_->hide();
                }
            }
        }
        
        return NUIComponent::onMouseEvent(event);
    }
    
    void onRender(NUIRenderer& renderer) override
    {
        // Draw background
        renderer.fillRect(getBounds(), NUIColor::fromHex(0xff1a1d22));
        
        // Render children first (UI components)
        renderChildren(renderer);
        
        // Only draw test text if context menu is not visible (to avoid conflicts)
        if (!contextMenu_ || !contextMenu_->isVisible()) {
            // Draw test text AFTER children so it appears on top
            renderer.drawText("TEST TEXT VISIBLE?", NUIPoint(60, 80), 24.0f, NUIColor(1.0f, 1.0f, 1.0f, 1.0f)); // White text
            
            // Draw multiple test texts at different positions
            renderer.drawText("TOP LEFT", NUIPoint(10, 20), 16.0f, NUIColor(1.0f, 0.0f, 0.0f, 1.0f)); // Red text
            renderer.drawText("CENTER", NUIPoint(150, 150), 20.0f, NUIColor(0.0f, 1.0f, 0.0f, 1.0f)); // Green text
            renderer.drawText("BOTTOM RIGHT", NUIPoint(200, 250), 14.0f, NUIColor(0.0f, 0.0f, 1.0f, 1.0f)); // Blue text
        }
    }
};

int main()
{
    std::cout << "NomadUI New Components Demo" << std::endl;
    std::cout << "===========================" << std::endl;
    
    // Create window
    NUIWindowWin32 window;
    if (!window.create("NomadUI New Components Demo", 400, 300))
    {
        std::cerr << "Failed to create window" << std::endl;
        return -1;
    }
    
    // Create OpenGL renderer
    auto renderer = std::make_unique<NUIRendererGL>();
    if (!renderer->initialize(400, 300))
    {
        std::cerr << "Failed to initialize renderer" << std::endl;
        return -1;
    }
    
    // Create demo component
    auto demo = std::make_shared<NewComponentsDemo>();
    demo->setBounds(NUIRect(0, 0, 400, 300));
    window.setRootComponent(demo.get());
    window.setRenderer(renderer.get());
    
    // Show window
    window.show();
    
    // Main loop
    while (window.processEvents())
    {
        // Clear screen
        renderer->beginFrame();
        renderer->clear(NUIColor::fromHex(0xff1a1d22));
        
        // Render demo
        demo->onRender(*renderer);
        
        // Present frame
        renderer->endFrame();
        window.swapBuffers();
        
        // Small delay to prevent 100% CPU usage
        std::this_thread::sleep_for(std::chrono::milliseconds(16)); // ~60 FPS
    }
    
    std::cout << "Demo completed successfully!" << std::endl;
    return 0;
}
