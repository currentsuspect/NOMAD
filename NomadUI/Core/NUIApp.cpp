#include "NUIApp.h"
#include <algorithm>
#include <thread>

namespace NomadUI {

NUIApp::NUIApp() {
}

NUIApp::~NUIApp() {
    shutdown();
}

// ============================================================================
// Lifecycle
// ============================================================================

bool NUIApp::initialize(int width, int height, const char* title) {
    if (initialized_) {
        return false;
    }
    
    width_ = width;
    height_ = height;
    
    // Create renderer
    renderer_ = createRenderer();
    if (!renderer_) {
        return false;
    }
    
    if (!renderer_->initialize(width, height)) {
        return false;
    }
    
    // Initialize timing
    lastFrameTime_ = std::chrono::high_resolution_clock::now();
    
    initialized_ = true;
    return true;
}

void NUIApp::shutdown() {
    if (!initialized_) {
        return;
    }
    
    running_ = false;
    
    // Cleanup
    rootComponent_.reset();
    focusedComponent_.reset();
    hoveredComponent_.reset();
    
    if (renderer_) {
        renderer_->shutdown();
        renderer_.reset();
    }
    
    initialized_ = false;
}

void NUIApp::run() {
    if (!initialized_) {
        return;
    }
    
    running_ = true;
    
    while (running_) {
        // Calculate delta time
        auto currentTime = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> elapsed = currentTime - lastFrameTime_;
        deltaTime_ = elapsed.count();
        lastFrameTime_ = currentTime;
        
        // Update FPS
        if (deltaTime_ > 0.0) {
            currentFPS_ = static_cast<float>(1.0 / deltaTime_);
        }
        
        // Process platform events
        processEvents();
        
        // Update
        update(deltaTime_);
        
        // Render
        render();
        
        // Limit frame rate
        double targetFrameTime = 1.0 / targetFPS_;
        if (deltaTime_ < targetFrameTime) {
            double sleepTime = targetFrameTime - deltaTime_;
            std::this_thread::sleep_for(
                std::chrono::duration<double>(sleepTime)
            );
        }
    }
}

void NUIApp::quit() {
    running_ = false;
}

// ============================================================================
// Components
// ============================================================================

void NUIApp::setRootComponent(std::shared_ptr<NUIComponent> root) {
    rootComponent_ = root;
    
    if (rootComponent_) {
        rootComponent_->setBounds(0, 0, 
            static_cast<float>(width_), 
            static_cast<float>(height_));
    }
}

// ============================================================================
// Rendering
// ============================================================================

void NUIApp::setTargetFPS(int fps) {
    targetFPS_ = std::max(1, std::min(fps, 240));
    frameTime_ = 1.0 / targetFPS_;
}

// ============================================================================
// Events
// ============================================================================

void NUIApp::setFocusedComponent(std::shared_ptr<NUIComponent> component) {
    if (focusedComponent_ == component) {
        return;
    }
    
    // Notify old focused component
    if (focusedComponent_) {
        focusedComponent_->setFocused(false);
    }
    
    focusedComponent_ = component;
    
    // Notify new focused component
    if (focusedComponent_) {
        focusedComponent_->setFocused(true);
    }
}

// ============================================================================
// Protected Methods
// ============================================================================

void NUIApp::processEvents() {
    // Platform-specific event processing
    // This will be implemented in platform-specific subclasses
    // or through a platform abstraction layer
}

void NUIApp::update(double deltaTime) {
    // Update root component
    if (rootComponent_) {
        rootComponent_->onUpdate(deltaTime);
    }
    
    // Call user update callback
    if (onUpdate) {
        onUpdate(deltaTime);
    }
}

void NUIApp::render() {
    if (!renderer_ || !rootComponent_) {
        return;
    }
    
    // Begin frame
    renderer_->beginFrame();
    
    // Clear background
    auto theme = rootComponent_->getTheme();
    if (theme) {
        renderer_->clear(theme.get()->getBackground());
    } else {
        renderer_->clear(NUIColor::black());
    }
    
    // Render root component
    rootComponent_->onRender(*renderer_);
    
    // Call user render callback
    if (onRender) {
        onRender();
    }
    
    // End frame
    renderer_->endFrame();
}

void NUIApp::handleMouseEvent(const NUIMouseEvent& event) {
    if (!rootComponent_) {
        return;
    }
    
    // Dispatch event to root component
    // The component will handle hover state internally
    rootComponent_->onMouseEvent(event);
    
    // Handle focus on click
    if (event.pressed) {
        // For now, just focus the root component
        // TODO: Implement proper hit testing
        setFocusedComponent(rootComponent_);
    }
}

void NUIApp::handleKeyEvent(const NUIKeyEvent& event) {
    // Dispatch to focused component
    if (focusedComponent_) {
        focusedComponent_->onKeyEvent(event);
    }
}

void NUIApp::handleResize(int width, int height) {
    width_ = width;
    height_ = height;
    
    if (renderer_) {
        renderer_->resize(width, height);
    }
    
    if (rootComponent_) {
        rootComponent_->setBounds(0, 0, 
            static_cast<float>(width), 
            static_cast<float>(height));
    }
}

} // namespace NomadUI
