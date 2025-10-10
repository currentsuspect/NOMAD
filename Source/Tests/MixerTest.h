#pragma once

#include <JuceHeader.h>
#include "../Audio/Mixer.h"

/**
 * Simple test application for the Mixer class.
 */
class MixerTest : public juce::Component,
                  public juce::Timer
{
public:
    MixerTest()
    {
        // Set up the UI
        addAndMakeVisible(gainSlider);
        gainSlider.setRange(0.0, 1.0, 0.01);
        gainSlider.setValue(0.7);
        gainSlider.onValueChange = [this] {
            if (auto* channel = mixer.getChannel(0))
                channel->setGain(static_cast<float>(gainSlider.getValue()));
        };
        
        addAndMakeVisible(panSlider);
        panSlider.setRange(-1.0, 1.0, 0.01);
        panSlider.setValue(0.0);
        panSlider.onValueChange = [this] {
            if (auto* channel = mixer.getChannel(0))
                channel->setPan(static_cast<float>(panSlider.getValue()));
        };
        
        addAndMakeVisible(muteButton);
        muteButton.setButtonText("Mute");
        muteButton.onClick = [this] {
            if (auto* channel = mixer.getChannel(0))
                channel->setMute(muteButton.getToggleState());
        };
        
        addAndMakeVisible(soloButton);
        soloButton.setButtonText("Solo");
        soloButton.onClick = [this] {
            if (auto* channel = mixer.getChannel(0))
                channel->setSolo(soloButton.getToggleState());
            
            // Update solo states
            mixer.handleSoloStateChanged();
        };
        
        // Set up the audio
        audioDeviceManager.initialiseWithDefaultDevices(0, 2); // No inputs, 2 outputs
        audioDeviceManager.addAudioCallback(&mixer);
        
        // Add a test tone generator
        startTimerHz(30); // Update meters at 30fps
    }
    
    ~MixerTest() override
    {
        audioDeviceManager.removeAudioCallback(&mixer);
    }
    
    void paint(juce::Graphics& g) override
    {
        g.fillAll(juce::Colour(0xff1e1e1e));
        
        // Draw meters
        if (auto* channel = mixer.getChannel(0))
        {
            const float peakLevel = channel->getPeakLevel();
            const float rmsLevel = channel->getRMSLevel();
            
            // Draw peak meter
            g.setColour(juce::Colours::red.withAlpha(0.7f));
            float peakHeight = juce::jmap(peakLevel, -60.0f, 0.0f, 0.0f, static_cast<float>(getHeight() - 20));
            g.fillRect(50.0f, getHeight() - 20.0f - peakHeight, 30.0f, peakHeight);
            
            // Draw RMS meter
            g.setColour(juce::Colours::green.withAlpha(0.7f));
            float rmsHeight = juce::jmap(rmsLevel, -60.0f, 0.0f, 0.0f, static_cast<float>(getHeight() - 20));
            g.fillRect(90.0f, getHeight() - 20.0f - rmsHeight, 30.0f, rmsHeight);
            
            // Draw labels
            g.setColour(juce::Colours::white);
            g.drawText("Peak", 50, getHeight() - 20, 30, 20, juce::Justification::centred);
            g.drawText("RMS", 90, getHeight() - 20, 30, 20, juce::Justification::centred);
        }
    }
    
    void resized() override
    {
        auto area = getLocalBounds().reduced(10);
        
        auto topArea = area.removeFromTop(30);
        gainSlider.setBounds(topArea.removeFromLeft(200));
        panSlider.setBounds(topArea.removeFromLeft(200));
        
        auto buttonArea = area.removeFromTop(30);
        muteButton.setBounds(buttonArea.removeFromLeft(100));
        soloButton.setBounds(buttonArea.removeFromLeft(100));
    }
    
    void timerCallback() override
    {
        // Update the meters
        repaint();
    }
    
private:
    juce::AudioDeviceManager audioDeviceManager;
    Mixer mixer;
    
    juce::Slider gainSlider;
    juce::Slider panSlider;
    juce::TextButton muteButton;
    juce::TextButton soloButton;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MixerTest)
};
