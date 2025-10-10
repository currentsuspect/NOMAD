#pragma once

#include <JuceHeader.h>

/**
 * Centralized GPU context manager for the entire application.
 * Manages a single OpenGL context shared across all components.
 * This ensures perfect synchronization, no redundant setup/destruction,
 * and easier VSync and performance tracking.
 */
class GPUContextManager
{
public:
    static GPUContextManager& getInstance()
    {
        static GPUContextManager instance;
        return instance;
    }
    
    // Attach the shared OpenGL context to a root component
    void attachToComponent(juce::Component* rootComponent)
    {
        if (rootComponent != nullptr && !openGLContext.isAttached())
        {
            openGLContext.setSwapInterval(1); // Enable VSync for smooth 60 FPS
            openGLContext.attachTo(*rootComponent);
            attachedComponent = rootComponent;
        }
    }
    
    // Detach the OpenGL context
    void detach()
    {
        if (openGLContext.isAttached())
        {
            openGLContext.detach();
            attachedComponent = nullptr;
        }
    }
    
    // Check if context is attached
    bool isAttached() const
    {
        return openGLContext.isAttached();
    }
    
    // Get the OpenGL context (for advanced usage)
    juce::OpenGLContext& getContext()
    {
        return openGLContext;
    }
    
    // Register a component for rendering notifications
    void registerComponent(juce::Component* component)
    {
        if (component != nullptr)
        {
            registeredComponents.addIfNotAlreadyThere(component);
        }
    }
    
    // Unregister a component
    void unregisterComponent(juce::Component* component)
    {
        registeredComponents.removeAllInstancesOf(component);
    }
    
    // Enable/disable rendering for a specific component
    void setComponentRenderingActive(juce::Component* component, bool shouldRender)
    {
        if (shouldRender)
        {
            activeComponents.addIfNotAlreadyThere(component);
        }
        else
        {
            activeComponents.removeAllInstancesOf(component);
        }
    }
    
    // Check if a component is actively rendering
    bool isComponentRenderingActive(juce::Component* component) const
    {
        return activeComponents.contains(component);
    }
    
private:
    GPUContextManager() = default;
    ~GPUContextManager()
    {
        detach();
    }
    
    // Prevent copying
    GPUContextManager(const GPUContextManager&) = delete;
    GPUContextManager& operator=(const GPUContextManager&) = delete;
    
    juce::OpenGLContext openGLContext;
    juce::Component* attachedComponent = nullptr;
    juce::Array<juce::Component*> registeredComponents;
    juce::Array<juce::Component*> activeComponents;
};
