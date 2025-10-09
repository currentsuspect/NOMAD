/**
 * @file AudioEngine.h
 * @brief Core audio engine implementation
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

namespace nomad::audio
{
    /**
     * @class AudioEngine
     * @brief High-performance audio engine with modular routing
     * 
     * The AudioEngine provides real-time, low-latency audio processing
     * with gapless playback, automatic latency compensation, and SIMD optimization.
     */
    class AudioEngine : public juce::AudioIODeviceCallback
    {
    public:
        /**
         * @brief Constructor
         * @param sampleRate Target sample rate
         * @param bufferSize Target buffer size
         */
        AudioEngine(double sampleRate = 44100.0, int bufferSize = 512);
        
        /**
         * @brief Destructor
         */
        ~AudioEngine();
        
        /**
         * @brief Initialize the audio engine
         * @return true if successful
         */
        bool initialize();
        
        /**
         * @brief Shutdown the audio engine
         */
        void shutdown();
        
        // AudioIODeviceCallback interface
        void audioDeviceIOCallbackWithContext(const float* const* inputChannelData,
                                            int numInputChannels,
                                            float* const* outputChannelData,
                                            int numOutputChannels,
                                            int numSamples,
                                            const juce::AudioIODeviceCallbackContext& context) override;
        
        void audioDeviceAboutToStart(juce::AudioIODevice* device) override;
        void audioDeviceStopped() override;
        
        /**
         * @brief Get the audio processor graph
         * @return Reference to the audio graph
         */
        juce::AudioProcessorGraph& getGraph() { return audioGraph; }
        
        /**
         * @brief Get current sample rate
         * @return Sample rate in Hz
         */
        double getSampleRate() const { return currentSampleRate; }
        
        /**
         * @brief Get current buffer size
         * @return Buffer size in samples
         */
        int getBufferSize() const { return currentBufferSize; }
        
        /**
         * @brief Set buffer size
         * @param newBufferSize New buffer size
         */
        void setBufferSize(int newBufferSize);
        
        /**
         * @brief Set sample rate
         * @param newSampleRate New sample rate
         */
        void setSampleRate(double newSampleRate);
        
        /**
         * @brief Add a processor to the graph
         * @param processor The processor to add
         * @return Node ID if successful, -1 if failed
         */
        juce::AudioProcessorGraph::NodeID addProcessor(std::unique_ptr<juce::AudioProcessor> processor);
        
        /**
         * @brief Remove a processor from the graph
         * @param nodeId Node ID to remove
         * @return true if successful
         */
        bool removeProcessor(juce::AudioProcessorGraph::NodeID nodeId);
        
        /**
         * @brief Connect two processors
         * @param sourceNode Source node ID
         * @param sourceChannel Source channel
         * @param destNode Destination node ID
         * @param destChannel Destination channel
         * @return true if successful
         */
        bool connectProcessors(juce::AudioProcessorGraph::NodeID sourceNode, int sourceChannel,
                             juce::AudioProcessorGraph::NodeID destNode, int destChannel);
        
        /**
         * @brief Disconnect processors
         * @param sourceNode Source node ID
         * @param destNode Destination node ID
         * @return true if successful
         */
        bool disconnectProcessors(juce::AudioProcessorGraph::NodeID sourceNode,
                                juce::AudioProcessorGraph::NodeID destNode);
        
        /**
         * @brief Get latency compensation value
         * @return Latency in samples
         */
        int getLatencyCompensation() const;
        
        /**
         * @brief Enable/disable double buffering
         * @param enabled True to enable
         */
        void setDoubleBufferingEnabled(bool enabled);
        
        /**
         * @brief Check if double buffering is enabled
         * @return True if enabled
         */
        bool isDoubleBufferingEnabled() const { return doubleBufferingEnabled; }
        
        /**
         * @brief Add a real-time callback
         * @param callback Function to call on audio thread
         */
        void addRealtimeCallback(std::function<void()> callback);
        
        /**
         * @brief Remove a real-time callback
         * @param callback Function to remove
         */
        void removeRealtimeCallback(std::function<void()> callback);
        
        /**
         * @brief Get performance statistics
         * @return Performance stats structure
         */
        struct PerformanceStats
        {
            double cpuUsage = 0.0;
            double maxCpuUsage = 0.0;
            int bufferUnderruns = 0;
            int bufferOverruns = 0;
            double averageLatency = 0.0;
        };
        
        PerformanceStats getPerformanceStats() const;
        
    private:
        // Core audio components
        juce::AudioProcessorGraph audioGraph;
        std::unique_ptr<juce::AudioIODevice> audioDevice;
        std::unique_ptr<juce::AudioDeviceManager> deviceManager;
        
        // Audio parameters
        std::atomic<double> currentSampleRate{44100.0};
        std::atomic<int> currentBufferSize{512};
        std::atomic<bool> doubleBufferingEnabled{true};
        
        // Buffer management
        std::vector<juce::AudioBuffer<float>> bufferPool;
        std::queue<juce::AudioBuffer<float>*> availableBuffers;
        std::mutex bufferPoolMutex;
        
        // Performance monitoring
        mutable std::atomic<double> cpuUsage{0.0};
        mutable std::atomic<double> maxCpuUsage{0.0};
        mutable std::atomic<int> bufferUnderruns{0};
        mutable std::atomic<int> bufferOverruns{0};
        
        // Real-time callbacks
        std::vector<std::function<void()>> realtimeCallbacks;
        std::mutex callbackMutex;
        
        // Internal methods
        juce::AudioBuffer<float>* getBuffer();
        void returnBuffer(juce::AudioBuffer<float>* buffer);
        void updatePerformanceStats(double processingTime, int bufferSize);
        void processRealtimeCallbacks();
        
        // SIMD optimization
        void processWithSIMD(juce::AudioBuffer<float>& buffer);
    };
}