#pragma once

#include <JuceHeader.h>

/**
 * Performance monitoring system with CPU/GPU split tracking.
 * Tracks frame time, FPS, CPU render time, GPU render time, and paint count.
 * Helps identify bottlenecks in the rendering pipeline.
 */
class PerformanceMonitor
{
public:
    static PerformanceMonitor& getInstance()
    {
        static PerformanceMonitor instance;
        return instance;
    }
    
    // Frame timing
    void beginFrame()
    {
        frameStartTime = juce::Time::getMillisecondCounterHiRes();
        cpuStartTime = frameStartTime;
    }
    
    void endCPUPhase()
    {
        cpuEndTime = juce::Time::getMillisecondCounterHiRes();
        lastCPUTime = cpuEndTime - cpuStartTime;
    }
    
    void endFrame()
    {
        double frameEndTime = juce::Time::getMillisecondCounterHiRes();
        lastFrameTime = frameEndTime - frameStartTime;
        lastGPUTime = frameEndTime - cpuEndTime;
        
        // Update FPS calculation
        frameCount++;
        double timeSinceLastUpdate = frameEndTime - lastFPSUpdateTime;
        
        if (timeSinceLastUpdate >= 1000.0) // Update FPS every second
        {
            currentFPS = frameCount / (timeSinceLastUpdate / 1000.0);
            frameCount = 0;
            lastFPSUpdateTime = frameEndTime;
        }
        
        // Update averages
        updateAverages();
    }
    
    // Paint tracking
    void incrementPaintCount()
    {
        currentFramePaintCount++;
    }
    
    void resetPaintCount()
    {
        lastPaintCount = currentFramePaintCount;
        currentFramePaintCount = 0;
    }
    
    // Getters
    double getFPS() const { return currentFPS; }
    double getFrameTime() const { return lastFrameTime; }
    double getCPUTime() const { return lastCPUTime; }
    double getGPUTime() const { return lastGPUTime; }
    int getPaintCount() const { return lastPaintCount; }
    
    double getAverageFrameTime() const { return avgFrameTime; }
    double getAverageCPUTime() const { return avgCPUTime; }
    double getAverageGPUTime() const { return avgGPUTime; }
    
    // Get formatted stats string
    juce::String getStatsString() const
    {
        juce::String stats;
        stats << "FPS: " << juce::String(currentFPS, 1) << "\n";
        stats << "Frame: " << juce::String(lastFrameTime, 2) << " ms\n";
        stats << "CPU: " << juce::String(lastCPUTime, 2) << " ms\n";
        stats << "GPU: " << juce::String(lastGPUTime, 2) << " ms\n";
        stats << "Paints: " << juce::String(lastPaintCount);
        return stats;
    }
    
    // Reset all stats
    void reset()
    {
        frameCount = 0;
        currentFPS = 0.0;
        lastFrameTime = 0.0;
        lastCPUTime = 0.0;
        lastGPUTime = 0.0;
        lastPaintCount = 0;
        currentFramePaintCount = 0;
        avgFrameTime = 0.0;
        avgCPUTime = 0.0;
        avgGPUTime = 0.0;
        lastFPSUpdateTime = juce::Time::getMillisecondCounterHiRes();
    }
    
private:
    PerformanceMonitor()
    {
        reset();
    }
    
    ~PerformanceMonitor() = default;
    
    // Prevent copying
    PerformanceMonitor(const PerformanceMonitor&) = delete;
    PerformanceMonitor& operator=(const PerformanceMonitor&) = delete;
    
    void updateAverages()
    {
        const double smoothing = 0.9; // Exponential moving average
        avgFrameTime = avgFrameTime * smoothing + lastFrameTime * (1.0 - smoothing);
        avgCPUTime = avgCPUTime * smoothing + lastCPUTime * (1.0 - smoothing);
        avgGPUTime = avgGPUTime * smoothing + lastGPUTime * (1.0 - smoothing);
    }
    
    // Timing
    double frameStartTime = 0.0;
    double cpuStartTime = 0.0;
    double cpuEndTime = 0.0;
    double lastFPSUpdateTime = 0.0;
    
    // Stats
    int frameCount = 0;
    double currentFPS = 0.0;
    double lastFrameTime = 0.0;
    double lastCPUTime = 0.0;
    double lastGPUTime = 0.0;
    int lastPaintCount = 0;
    int currentFramePaintCount = 0;
    
    // Averages
    double avgFrameTime = 0.0;
    double avgCPUTime = 0.0;
    double avgGPUTime = 0.0;
};
