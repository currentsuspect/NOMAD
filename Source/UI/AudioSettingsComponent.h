#pragma once

#include <JuceHeader.h>
#include "WindowControlButton.h"

/**
 * UI component for audio device settings.
 * Provides controls for device selection, buffer size, and sample rate.
 */
class AudioSettingsComponent : public juce::Component
{
public:
    AudioSettingsComponent(juce::AudioDeviceManager& deviceManager);
    ~AudioSettingsComponent() override;
    
    void paint(juce::Graphics& g) override;
    void resized() override;
    void mouseDown(const juce::MouseEvent& event) override;
    void mouseDrag(const juce::MouseEvent& event) override;
    
private:
    juce::AudioDeviceManager& audioDeviceManager;
    std::unique_ptr<juce::AudioDeviceSelectorComponent> deviceSelector;
    WindowControlButton closeButton { WindowControlButton::Type::Close };
    juce::ComponentDragger dragger;
    juce::Rectangle<int> titleBarArea;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AudioSettingsComponent)
};
