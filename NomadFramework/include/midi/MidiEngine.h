/**
 * @file MidiEngine.h
 * @brief Real-time MIDI engine with zero-copy event dispatching
 * @author Nomad Framework Team
 */

#pragma once

#include <JuceHeader.h>
#include <memory>
#include <vector>
#include <atomic>
#include <mutex>
#include <queue>
#include <functional>
#include <unordered_map>

namespace nomad::midi
{
    /**
     * @struct MidiEvent
     * @brief Lightweight MIDI event structure for zero-copy operations
     */
    struct MidiEvent
    {
        uint8_t status;
        uint8_t data1;
        uint8_t data2;
        double timestamp;
        int channel;
        
        MidiEvent() = default;
        MidiEvent(uint8_t s, uint8_t d1, uint8_t d2, double t, int ch)
            : status(s), data1(d1), data2(d2), timestamp(t), channel(ch) {}
    };
    
    /**
     * @class MidiEngine
     * @brief High-performance MIDI engine with real-time routing
     * 
     * Provides zero-copy MIDI event dispatching, clock sync, quantization,
     * and tight integration with transport and automation systems.
     */
    class MidiEngine : public juce::MidiInputCallback
    {
    public:
        /**
         * @brief Constructor
         */
        MidiEngine();
        
        /**
         * @brief Destructor
         */
        ~MidiEngine();
        
        /**
         * @brief Initialize the MIDI engine
         * @return true if successful
         */
        bool initialize();
        
        /**
         * @brief Shutdown the MIDI engine
         */
        void shutdown();
        
        // MidiInputCallback interface
        void handleIncomingMidiMessage(juce::MidiInput* source,
                                     const juce::MidiMessage& message) override;
        
        /**
         * @brief Add MIDI input device
         * @param deviceName Name of the MIDI device
         * @return true if successful
         */
        bool addMidiInput(const juce::String& deviceName);
        
        /**
         * @brief Remove MIDI input device
         * @param deviceName Name of the MIDI device
         */
        void removeMidiInput(const juce::String& deviceName);
        
        /**
         * @brief Add MIDI output device
         * @param deviceName Name of the MIDI device
         * @return true if successful
         */
        bool addMidiOutput(const juce::String& deviceName);
        
        /**
         * @brief Remove MIDI output device
         * @param deviceName Name of the MIDI device
         */
        void removeMidiOutput(const juce::String& deviceName);
        
        /**
         * @brief Send MIDI message
         * @param message MIDI message to send
         * @param outputDevice Output device name (empty for all)
         */
        void sendMidiMessage(const juce::MidiMessage& message, const juce::String& outputDevice = "");
        
        /**
         * @brief Send MIDI event
         * @param event MIDI event to send
         * @param outputDevice Output device name (empty for all)
         */
        void sendMidiEvent(const MidiEvent& event, const juce::String& outputDevice = "");
        
        /**
         * @brief Add MIDI event callback
         * @param callback Function to call when MIDI events are received
         */
        void addMidiEventCallback(std::function<void(const MidiEvent&)> callback);
        
        /**
         * @brief Remove MIDI event callback
         * @param callback Function to remove
         */
        void removeMidiEventCallback(std::function<void(const MidiEvent&)> callback);
        
        /**
         * @brief Set clock sync enabled
         * @param enabled True to enable clock sync
         */
        void setClockSyncEnabled(bool enabled);
        
        /**
         * @brief Check if clock sync is enabled
         * @return True if enabled
         */
        bool isClockSyncEnabled() const { return clockSyncEnabled.load(); }
        
        /**
         * @brief Set quantization enabled
         * @param enabled True to enable quantization
         */
        void setQuantizationEnabled(bool enabled);
        
        /**
         * @brief Check if quantization is enabled
         * @return True if enabled
         */
        bool isQuantizationEnabled() const { return quantizationEnabled.load(); }
        
        /**
         * @brief Set quantization grid
         * @param gridSize Grid size in beats (e.g., 0.25 for quarter notes)
         */
        void setQuantizationGrid(double gridSize);
        
        /**
         * @brief Get quantization grid
         * @return Grid size in beats
         */
        double getQuantizationGrid() const { return quantizationGrid.load(); }
        
        /**
         * @brief Set current tempo
         * @param tempo Tempo in BPM
         */
        void setTempo(double tempo);
        
        /**
         * @brief Get current tempo
         * @return Tempo in BPM
         */
        double getTempo() const { return currentTempo.load(); }
        
        /**
         * @brief Set current time position
         * @param time Time in beats
         */
        void setTimePosition(double time);
        
        /**
         * @brief Get current time position
         * @return Time in beats
         */
        double getTimePosition() const { return currentTimePosition.load(); }
        
        /**
         * @brief Process MIDI events (call from audio thread)
         * @param numSamples Number of samples to process
         */
        void processMidiEvents(int numSamples);
        
        /**
         * @brief Get available MIDI input devices
         * @return Vector of device names
         */
        std::vector<juce::String> getAvailableInputDevices() const;
        
        /**
         * @brief Get available MIDI output devices
         * @return Vector of device names
         */
        std::vector<juce::String> getAvailableOutputDevices() const;
        
        /**
         * @brief Get MIDI statistics
         * @return Statistics structure
         */
        struct MidiStats
        {
            int eventsProcessed = 0;
            int eventsDropped = 0;
            double averageLatency = 0.0;
            int activeInputs = 0;
            int activeOutputs = 0;
        };
        
        MidiStats getMidiStats() const;
        
    private:
        // MIDI devices
        std::vector<std::unique_ptr<juce::MidiInput>> midiInputs;
        std::vector<std::unique_ptr<juce::MidiOutput>> midiOutputs;
        std::unordered_map<juce::String, juce::MidiOutput*> outputDeviceMap;
        
        // Event processing
        std::queue<MidiEvent> eventQueue;
        std::mutex eventQueueMutex;
        std::vector<std::function<void(const MidiEvent&)>> eventCallbacks;
        std::mutex callbackMutex;
        
        // Clock and timing
        std::atomic<bool> clockSyncEnabled{false};
        std::atomic<bool> quantizationEnabled{false};
        std::atomic<double> quantizationGrid{0.25}; // Quarter notes
        std::atomic<double> currentTempo{120.0};
        std::atomic<double> currentTimePosition{0.0};
        
        // Statistics
        mutable std::atomic<int> eventsProcessed{0};
        mutable std::atomic<int> eventsDropped{0};
        mutable std::atomic<double> averageLatency{0.0};
        
        // Internal methods
        void processClockMessage(const juce::MidiMessage& message);
        void processQuantization(MidiEvent& event);
        double getCurrentTimeInBeats() const;
        void updateStatistics(const MidiEvent& event, double latency);
    };
}