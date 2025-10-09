/**
 * @file AudioEngine.cpp
 * @brief Core audio engine implementation
 */

#include "audio/AudioEngine.h"
#include <algorithm>
#include <chrono>

namespace nomad::audio
{
    AudioEngine::AudioEngine(double sampleRate, int bufferSize)
        : currentSampleRate(sampleRate), currentBufferSize(bufferSize)
    {
        // Initialize buffer pool
        const int poolSize = 8;
        bufferPool.reserve(poolSize);
        for (int i = 0; i < poolSize; ++i)
        {
            bufferPool.emplace_back(2, bufferSize); // Stereo buffers
            availableBuffers.push(&bufferPool[i]);
        }
    }
    
    AudioEngine::~AudioEngine()
    {
        shutdown();
    }
    
    bool AudioEngine::initialize()
    {
        try
        {
            // Initialize device manager
            deviceManager = std::make_unique<juce::AudioDeviceManager>();
            
            // Setup audio device
            juce::String error = deviceManager->initialise(0, 2, nullptr, true);
            if (error.isNotEmpty())
            {
                return false;
            }
            
            // Set sample rate and buffer size
            auto device = deviceManager->getCurrentAudioDevice();
            if (device)
            {
                device->setCurrentSampleRate(currentSampleRate.load());
                device->setCurrentBufferSizeSamples(currentBufferSize.load());
            }
            
            // Add this as the audio callback
            deviceManager->addAudioCallback(this);
            
            // Initialize the audio graph
            audioGraph.prepareToPlay(currentSampleRate.load(), currentBufferSize.load());
            
            return true;
        }
        catch (const std::exception& e)
        {
            return false;
        }
    }
    
    void AudioEngine::shutdown()
    {
        if (deviceManager)
        {
            deviceManager->removeAudioCallback(this);
            deviceManager->closeAudioDevice();
        }
        
        audioGraph.releaseResources();
    }
    
    void AudioEngine::audioDeviceIOCallbackWithContext(const float* const* inputChannelData,
                                                      int numInputChannels,
                                                      float* const* outputChannelData,
                                                      int numOutputChannels,
                                                      int numSamples,
                                                      const juce::AudioIODeviceCallbackContext& context)
    {
        // Performance timing
        auto startTime = std::chrono::high_resolution_clock::now();
        
        // Clear output buffers
        for (int i = 0; i < numOutputChannels; ++i)
        {
            juce::FloatVectorOperations::clear(outputChannelData[i], numSamples);
        }
        
        // Process audio graph
        juce::AudioBuffer<float> inputBuffer(const_cast<float* const*>(inputChannelData), 
                                           numInputChannels, numSamples);
        juce::AudioBuffer<float> outputBuffer(outputChannelData, numOutputChannels, numSamples);
        
        // Process with the audio graph
        audioGraph.processBlock(inputBuffer, outputBuffer);
        
        // Apply SIMD optimization if enabled
        if (doubleBufferingEnabled.load())
        {
            processWithSIMD(outputBuffer);
        }
        
        // Process real-time callbacks
        processRealtimeCallbacks();
        
        // Update performance stats
        auto endTime = std::chrono::high_resolution_clock::now();
        auto processingTime = std::chrono::duration<double, std::milli>(endTime - startTime).count();
        updatePerformanceStats(processingTime, numSamples);
    }
    
    void AudioEngine::audioDeviceAboutToStart(juce::AudioIODevice* device)
    {
        currentSampleRate = device->getCurrentSampleRate();
        currentBufferSize = device->getCurrentBufferSizeSamples();
        audioGraph.prepareToPlay(currentSampleRate, currentBufferSize);
    }
    
    void AudioEngine::audioDeviceStopped()
    {
        audioGraph.releaseResources();
    }
    
    void AudioEngine::setBufferSize(int newBufferSize)
    {
        currentBufferSize = newBufferSize;
        if (deviceManager && deviceManager->getCurrentAudioDevice())
        {
            deviceManager->getCurrentAudioDevice()->setCurrentBufferSizeSamples(newBufferSize);
        }
    }
    
    void AudioEngine::setSampleRate(double newSampleRate)
    {
        currentSampleRate = newSampleRate;
        if (deviceManager && deviceManager->getCurrentAudioDevice())
        {
            deviceManager->getCurrentAudioDevice()->setCurrentSampleRate(newSampleRate);
        }
    }
    
    juce::AudioProcessorGraph::NodeID AudioEngine::addProcessor(std::unique_ptr<juce::AudioProcessor> processor)
    {
        return audioGraph.addNode(std::move(processor));
    }
    
    bool AudioEngine::removeProcessor(juce::AudioProcessorGraph::NodeID nodeId)
    {
        return audioGraph.removeNode(nodeId);
    }
    
    bool AudioEngine::connectProcessors(juce::AudioProcessorGraph::NodeID sourceNode, int sourceChannel,
                                      juce::AudioProcessorGraph::NodeID destNode, int destChannel)
    {
        return audioGraph.addConnection({sourceNode, sourceChannel}, {destNode, destChannel});
    }
    
    bool AudioEngine::disconnectProcessors(juce::AudioProcessorGraph::NodeID sourceNode,
                                         juce::AudioProcessorGraph::NodeID destNode)
    {
        return audioGraph.removeConnection({sourceNode, 0}, {destNode, 0});
    }
    
    int AudioEngine::getLatencyCompensation() const
    {
        return audioGraph.getLatencySamples();
    }
    
    void AudioEngine::setDoubleBufferingEnabled(bool enabled)
    {
        doubleBufferingEnabled = enabled;
    }
    
    void AudioEngine::addRealtimeCallback(std::function<void()> callback)
    {
        std::lock_guard<std::mutex> lock(callbackMutex);
        realtimeCallbacks.push_back(callback);
    }
    
    void AudioEngine::removeRealtimeCallback(std::function<void()> callback)
    {
        std::lock_guard<std::mutex> lock(callbackMutex);
        realtimeCallbacks.erase(
            std::remove(realtimeCallbacks.begin(), realtimeCallbacks.end(), callback),
            realtimeCallbacks.end()
        );
    }
    
    AudioEngine::PerformanceStats AudioEngine::getPerformanceStats() const
    {
        PerformanceStats stats;
        stats.cpuUsage = cpuUsage.load();
        stats.maxCpuUsage = maxCpuUsage.load();
        stats.bufferUnderruns = bufferUnderruns.load();
        stats.bufferOverruns = bufferOverruns.load();
        stats.averageLatency = getLatencyCompensation() / currentSampleRate.load() * 1000.0; // ms
        return stats;
    }
    
    juce::AudioBuffer<float>* AudioEngine::getBuffer()
    {
        std::lock_guard<std::mutex> lock(bufferPoolMutex);
        if (!availableBuffers.empty())
        {
            auto buffer = availableBuffers.front();
            availableBuffers.pop();
            return buffer;
        }
        return nullptr;
    }
    
    void AudioEngine::returnBuffer(juce::AudioBuffer<float>* buffer)
    {
        std::lock_guard<std::mutex> lock(bufferPoolMutex);
        availableBuffers.push(buffer);
    }
    
    void AudioEngine::updatePerformanceStats(double processingTime, int bufferSize)
    {
        double targetTime = (bufferSize / currentSampleRate.load()) * 1000.0; // ms
        double currentCpuUsage = (processingTime / targetTime) * 100.0;
        
        cpuUsage = currentCpuUsage;
        
        double maxCpu = maxCpuUsage.load();
        if (currentCpuUsage > maxCpu)
        {
            maxCpuUsage = currentCpuUsage;
        }
        
        if (processingTime > targetTime * 1.1) // 10% tolerance
        {
            bufferUnderruns++;
        }
    }
    
    void AudioEngine::processRealtimeCallbacks()
    {
        std::lock_guard<std::mutex> lock(callbackMutex);
        for (auto& callback : realtimeCallbacks)
        {
            callback();
        }
    }
    
    void AudioEngine::processWithSIMD(juce::AudioBuffer<float>& buffer)
    {
        // SIMD-optimized processing
        // This is a placeholder for SIMD operations
        // In a real implementation, you would use JUCE's SIMD classes
        // or platform-specific SIMD instructions
        
        for (int channel = 0; channel < buffer.getNumChannels(); ++channel)
        {
            auto* channelData = buffer.getWritePointer(channel);
            int numSamples = buffer.getNumSamples();
            
            // Example SIMD processing (simplified)
            for (int i = 0; i < numSamples; i += 4)
            {
                // Process 4 samples at once using SIMD
                // This would be replaced with actual SIMD instructions
                for (int j = 0; j < 4 && (i + j) < numSamples; ++j)
                {
                    channelData[i + j] *= 1.0f; // Placeholder
                }
            }
        }
    }
}