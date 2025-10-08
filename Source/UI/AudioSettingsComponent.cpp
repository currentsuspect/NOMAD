#include "AudioSettingsComponent.h"

AudioSettingsComponent::AudioSettingsComponent(juce::AudioDeviceManager& deviceManager)
    : audioDeviceManager(deviceManager)
{
    // Purple theme color
    juce::Colour purpleGlow(0xffa855f7);
    
    // Create the audio device selector component
    deviceSelector = std::make_unique<juce::AudioDeviceSelectorComponent>(
        audioDeviceManager,
        0,     // Minimum input channels
        256,   // Maximum input channels
        0,     // Minimum output channels
        256,   // Maximum output channels
        true,  // Show MIDI input options
        true,  // Show MIDI output options
        true,  // Show channels as stereo pairs
        false  // Hide advanced options initially
    );
    
    // Apply purple theme colors
    deviceSelector->setColour(juce::ListBox::backgroundColourId, juce::Colour(0xff0d0e0f));
    deviceSelector->setColour(juce::ListBox::outlineColourId, purpleGlow.withAlpha(0.3f));
    deviceSelector->setColour(juce::ComboBox::backgroundColourId, juce::Colour(0xff1a1a1a));
    deviceSelector->setColour(juce::ComboBox::outlineColourId, purpleGlow.withAlpha(0.3f));
    deviceSelector->setColour(juce::ComboBox::buttonColourId, juce::Colour(0xff2a2a2a));
    deviceSelector->setColour(juce::ComboBox::arrowColourId, purpleGlow);
    deviceSelector->setColour(juce::ComboBox::textColourId, juce::Colour(0xffcccccc));
    deviceSelector->setColour(juce::TextButton::buttonColourId, juce::Colour(0xff1c1f23));
    deviceSelector->setColour(juce::TextButton::buttonOnColourId, purpleGlow.withAlpha(0.3f));
    deviceSelector->setColour(juce::TextButton::textColourOffId, juce::Colour(0xff888888));
    deviceSelector->setColour(juce::TextButton::textColourOnId, purpleGlow);
    deviceSelector->setColour(juce::Label::textColourId, juce::Colour(0xffcccccc));
    deviceSelector->setColour(juce::ToggleButton::textColourId, juce::Colour(0xffcccccc));
    deviceSelector->setColour(juce::ToggleButton::tickColourId, purpleGlow);
    deviceSelector->setColour(juce::ToggleButton::tickDisabledColourId, juce::Colour(0xff444444));
    
    addAndMakeVisible(deviceSelector.get());
    
    // Setup close button with custom WindowControlButton
    closeButton.onClick = [this]
    {
        // Close only the dialog window, not the main app
        if (auto* dw = findParentComponentOfClass<juce::DialogWindow>())
        {
            dw->exitModalState(0);
        }
    };
    addAndMakeVisible(closeButton);
    
    setSize(600, 450);
}

AudioSettingsComponent::~AudioSettingsComponent()
{
}

void AudioSettingsComponent::paint(juce::Graphics& g)
{
    juce::Colour purpleGlow(0xffa855f7);
    
    // Dark background
    g.fillAll(juce::Colour(0xff0d0e0f));
    
    // Custom title bar
    g.setColour(juce::Colour(0xff151618));
    g.fillRect(titleBarArea);
    
    // Title text - modern, sleek font
    g.setFont(juce::Font("Arial", 13.0f, juce::Font::plain));
    g.setColour(purpleGlow);
    g.drawText("AUDIO SETTINGS", titleBarArea.reduced(12, 0), 
               juce::Justification::centredLeft, true);
    
    // Separator under title bar
    g.setColour(purpleGlow.withAlpha(0.3f));
    g.drawLine(0, (float)titleBarArea.getBottom(), (float)getWidth(), 
               (float)titleBarArea.getBottom(), 2.0f);
    g.setColour(purpleGlow.withAlpha(0.5f));
    g.drawLine(0, (float)titleBarArea.getBottom(), (float)getWidth(), 
               (float)titleBarArea.getBottom(), 1.0f);
    
    // Purple glow border
    g.setColour(purpleGlow.withAlpha(0.3f));
    g.drawRect(getLocalBounds(), 2);
}

void AudioSettingsComponent::resized()
{
    auto bounds = getLocalBounds();
    
    // Title bar area
    titleBarArea = bounds.removeFromTop(40);
    
    // Close button in title bar (32px to match main window)
    closeButton.setBounds(titleBarArea.removeFromRight(32));
    
    // Device selector takes remaining space
    deviceSelector->setBounds(bounds.reduced(10));
}

void AudioSettingsComponent::mouseDown(const juce::MouseEvent& event)
{
    // Allow dragging from title bar
    if (titleBarArea.contains(event.getPosition()))
    {
        if (auto* dw = findParentComponentOfClass<juce::DialogWindow>())
        {
            dragger.startDraggingComponent(dw, event);
        }
    }
}

void AudioSettingsComponent::mouseDrag(const juce::MouseEvent& event)
{
    // Drag the dialog window
    if (titleBarArea.contains(event.getMouseDownPosition()))
    {
        if (auto* dw = findParentComponentOfClass<juce::DialogWindow>())
        {
            dragger.dragComponent(dw, event, nullptr);
        }
    }
}
