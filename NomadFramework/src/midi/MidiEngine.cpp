/**
 * @file MidiEngine.cpp
 * @brief Real-time MIDI engine implementation
 */

#include "midi/MidiEngine.h"
#include <algorithm>
#include <chrono>

namespace nomad::midi
{
    MidiEngine::MidiEngine()
    {
    }
    
    MidiEngine::~MidiEngine()
    {
        shutdown();
    }
    
    bool MidiEngine::initialize()
    {
        try
        {
            // Initialize MIDI devices
            auto inputDevices = juce::MidiInput::getAvailableDevices();
            auto outputDevices = juce::MidiOutput::getAvailableDevices();
            
            // Open default devices if available
            if (!inputDevices.isEmpty())
            {
                addMidiInput(inputDevices[0].name);
            }
            
            if (!outputDevices.isEmpty())
            {
                addMidiOutput(outputDevices[0].name);
            }
            
            return true;
        }
        catch (const std::exception& e)
        {
            return false;
        }
    }
    
    void MidiEngine::shutdown()
    {
        // Close all MIDI inputs
        for (auto& input : midiInputs)
        {
            input->stop();
        }
        midiInputs.clear();
        
        // Close all MIDI outputs
        for (auto& output : midiOutputs)
        {
            output->stopBackgroundThread();
        }
        midiOutputs.clear();
        outputDeviceMap.clear();
    }
    
    void MidiEngine::handleIncomingMidiMessage(juce::MidiInput* source,
                                             const juce::MidiMessage& message)
    {
        // Convert JUCE MIDI message to our event format
        MidiEvent event;
        event.status = message.getRawData()[0];
        event.data1 = message.getRawDataSize() > 1 ? message.getRawData()[1] : 0;
        event.data2 = message.getRawDataSize() > 2 ? message.getRawData()[2] : 0;
        event.timestamp = message.getTimeStamp();
        event.channel = message.getChannel();
        
        // Process clock messages
        if (clockSyncEnabled.load())
        {
            processClockMessage(message);
        }
        
        // Add to event queue
        {
            std::lock_guard<std::mutex> lock(eventQueueMutex);
            eventQueue.push(event);
        }
        
        // Update statistics
        updateStatistics(event, 0.0); // Latency calculation would go here
    }
    
    bool MidiEngine::addMidiInput(const juce::String& deviceName)
    {
        try
        {
            auto input = juce::MidiInput::openDevice(deviceName, this);
            if (input)
            {
                input->start();
                midiInputs.push_back(std::move(input));
                return true;
            }
        }
        catch (const std::exception& e)
        {
            // Log error
        }
        return false;
    }
    
    void MidiEngine::removeMidiInput(const juce::String& deviceName)
    {
        midiInputs.erase(
            std::remove_if(midiInputs.begin(), midiInputs.end(),
                [&deviceName](const std::unique_ptr<juce::MidiInput>& input)
                {
                    return input->getName() == deviceName;
                }),
            midiInputs.end()
        );
    }
    
    bool MidiEngine::addMidiOutput(const juce::String& deviceName)
    {
        try
        {
            auto output = juce::MidiOutput::openDevice(deviceName);
            if (output)
            {
                output->startBackgroundThread();
                midiOutputs.push_back(std::move(output));
                outputDeviceMap[deviceName] = midiOutputs.back().get();
                return true;
            }
        }
        catch (const std::exception& e)
        {
            // Log error
        }
        return false;
    }
    
    void MidiEngine::removeMidiOutput(const juce::String& deviceName)
    {
        auto it = outputDeviceMap.find(deviceName);
        if (it != outputDeviceMap.end())
        {
            outputDeviceMap.erase(it);
        }
        
        midiOutputs.erase(
            std::remove_if(midiOutputs.begin(), midiOutputs.end(),
                [&deviceName](const std::unique_ptr<juce::MidiOutput>& output)
                {
                    return output->getName() == deviceName;
                }),
            midiOutputs.end()
        );
    }
    
    void MidiEngine::sendMidiMessage(const juce::MidiMessage& message, const juce::String& outputDevice)
    {
        if (outputDevice.isEmpty())
        {
            // Send to all outputs
            for (auto& output : midiOutputs)
            {
                output->sendMessageNow(message);
            }
        }
        else
        {
            // Send to specific output
            auto it = outputDeviceMap.find(outputDevice);
            if (it != outputDeviceMap.end())
            {
                it->second->sendMessageNow(message);
            }
        }
    }
    
    void MidiEngine::sendMidiEvent(const MidiEvent& event, const juce::String& outputDevice)
    {
        juce::MidiMessage message(event.status, event.data1, event.data2, event.timestamp);
        sendMidiMessage(message, outputDevice);
    }
    
    void MidiEngine::addMidiEventCallback(std::function<void(const MidiEvent&)> callback)
    {
        std::lock_guard<std::mutex> lock(callbackMutex);
        eventCallbacks.push_back(callback);
    }
    
    void MidiEngine::removeMidiEventCallback(std::function<void(const MidiEvent&)> callback)
    {
        std::lock_guard<std::mutex> lock(callbackMutex);
        eventCallbacks.erase(
            std::remove(eventCallbacks.begin(), eventCallbacks.end(), callback),
            eventCallbacks.end()
        );
    }
    
    void MidiEngine::setClockSyncEnabled(bool enabled)
    {
        clockSyncEnabled = enabled;
    }
    
    void MidiEngine::setQuantizationEnabled(bool enabled)
    {
        quantizationEnabled = enabled;
    }
    
    void MidiEngine::setQuantizationGrid(double gridSize)
    {
        quantizationGrid = gridSize;
    }
    
    void MidiEngine::setTempo(double tempo)
    {
        currentTempo = tempo;
    }
    
    void MidiEngine::setTimePosition(double time)
    {
        currentTimePosition = time;
    }
    
    void MidiEngine::processMidiEvents(int numSamples)
    {
        std::queue<MidiEvent> eventsToProcess;
        
        // Move events from queue to processing queue
        {
            std::lock_guard<std::mutex> lock(eventQueueMutex);
            eventsToProcess = std::move(eventQueue);
        }
        
        // Process events
        while (!eventsToProcess.empty())
        {
            MidiEvent event = eventsToProcess.front();
            eventsToProcess.pop();
            
            // Apply quantization if enabled
            if (quantizationEnabled.load())
            {
                processQuantization(event);
            }
            
            // Call callbacks
            {
                std::lock_guard<std::mutex> lock(callbackMutex);
                for (auto& callback : eventCallbacks)
                {
                    callback(event);
                }
            }
            
            eventsProcessed++;
        }
    }
    
    std::vector<juce::String> MidiEngine::getAvailableInputDevices() const
    {
        std::vector<juce::String> devices;
        auto inputDevices = juce::MidiInput::getAvailableDevices();
        for (const auto& device : inputDevices)
        {
            devices.push_back(device.name);
        }
        return devices;
    }
    
    std::vector<juce::String> MidiEngine::getAvailableOutputDevices() const
    {
        std::vector<juce::String> devices;
        auto outputDevices = juce::MidiOutput::getAvailableDevices();
        for (const auto& device : outputDevices)
        {
            devices.push_back(device.name);
        }
        return devices;
    }
    
    MidiEngine::MidiStats MidiEngine::getMidiStats() const
    {
        MidiStats stats;
        stats.eventsProcessed = eventsProcessed.load();
        stats.eventsDropped = eventsDropped.load();
        stats.averageLatency = averageLatency.load();
        stats.activeInputs = static_cast<int>(midiInputs.size());
        stats.activeOutputs = static_cast<int>(midiOutputs.size());
        return stats;
    }
    
    void MidiEngine::processClockMessage(const juce::MidiMessage& message)
    {
        if (message.isMidiClock())
        {
            // Handle MIDI clock
            double beatsPerSecond = currentTempo.load() / 60.0;
            double timePerBeat = 1.0 / beatsPerSecond;
            currentTimePosition = currentTimePosition.load() + timePerBeat;
        }
        else if (message.isSongPositionPointer())
        {
            // Handle song position pointer
            int position = message.getSongPositionPointerMidiBeat();
            currentTimePosition = position * 0.25; // Assuming 16th note resolution
        }
    }
    
    void MidiEngine::processQuantization(MidiEvent& event)
    {
        if (!quantizationEnabled.load())
            return;
        
        double gridSize = quantizationGrid.load();
        double currentTime = getCurrentTimeInBeats();
        double quantizedTime = std::round(currentTime / gridSize) * gridSize;
        
        // Adjust event timestamp for quantization
        event.timestamp = quantizedTime;
    }
    
    double MidiEngine::getCurrentTimeInBeats() const
    {
        return currentTimePosition.load();
    }
    
    void MidiEngine::updateStatistics(const MidiEvent& event, double latency)
    {
        // Update average latency
        double currentAvg = averageLatency.load();
        int processed = eventsProcessed.load();
        averageLatency = (currentAvg * processed + latency) / (processed + 1);
    }
}