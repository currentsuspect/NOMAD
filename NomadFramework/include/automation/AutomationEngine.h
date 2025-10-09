/**
 * @file AutomationEngine.h
 * @brief Sample-accurate automation engine with LFO and keyframe support
 * @author Nomad Framework Team
 */

#pragma once

#include <JuceHeader.h>
#include <memory>
#include <vector>
#include <unordered_map>
#include <mutex>
#include <atomic>
#include <functional>
#include <deque>

namespace nomad::automation
{
    /**
     * @enum AutomationType
     * @brief Automation type enumeration
     */
    enum class AutomationType
    {
        Keyframe,
        LFO,
        Envelope,
        Curve
    };
    
    /**
     * @enum LFOType
     * @brief LFO type enumeration
     */
    enum class LFOType
    {
        Sine,
        Triangle,
        Square,
        Sawtooth,
        Random
    };
    
    /**
     * @struct AutomationPoint
     * @brief Automation point structure
     */
    struct AutomationPoint
    {
        double time;        // Time in seconds
        double value;       // Value (0.0 to 1.0)
        double curve;       // Curve shape (-1.0 to 1.0)
        
        AutomationPoint() = default;
        AutomationPoint(double t, double v, double c = 0.0)
            : time(t), value(v), curve(c) {}
    };
    
    /**
     * @struct LFOData
     * @brief LFO data structure
     */
    struct LFOData
    {
        LFOType type = LFOType::Sine;
        double frequency = 1.0;     // Frequency in Hz
        double amplitude = 1.0;     // Amplitude (0.0 to 1.0)
        double phase = 0.0;         // Phase offset (0.0 to 1.0)
        double offset = 0.0;        // DC offset (-1.0 to 1.0)
        bool syncToTempo = false;   // Sync to transport tempo
        double tempoMultiplier = 1.0; // Tempo multiplier
    };
    
    /**
     * @class AutomationLane
     * @brief Individual automation lane for a parameter
     */
    class AutomationLane
    {
    public:
        /**
         * @brief Constructor
         * @param parameterId Parameter ID to automate
         */
        AutomationLane(const std::string& parameterId);
        
        /**
         * @brief Destructor
         */
        ~AutomationLane();
        
        /**
         * @brief Get parameter ID
         * @return Parameter ID
         */
        const std::string& getParameterId() const { return parameterId; }
        
        /**
         * @brief Set automation type
         * @param type Automation type
         */
        void setAutomationType(AutomationType type);
        
        /**
         * @brief Get automation type
         * @return Automation type
         */
        AutomationType getAutomationType() const { return automationType; }
        
        /**
         * @brief Set enabled state
         * @param enabled True to enable automation
         */
        void setEnabled(bool enabled);
        
        /**
         * @brief Check if enabled
         * @return True if enabled
         */
        bool isEnabled() const { return enabled; }
        
        /**
         * @brief Add keyframe point
         * @param point Automation point
         */
        void addKeyframe(const AutomationPoint& point);
        
        /**
         * @brief Remove keyframe point
         * @param time Time of point to remove
         * @return True if removed
         */
        bool removeKeyframe(double time);
        
        /**
         * @brief Clear all keyframes
         */
        void clearKeyframes();
        
        /**
         * @brief Get keyframe at time
         * @param time Time to search for
         * @return Pointer to keyframe, nullptr if not found
         */
        const AutomationPoint* getKeyframeAtTime(double time) const;
        
        /**
         * @brief Get all keyframes
         * @return Vector of keyframes
         */
        std::vector<AutomationPoint> getKeyframes() const;
        
        /**
         * @brief Set LFO data
         * @param lfoData LFO data
         */
        void setLFOData(const LFOData& lfoData);
        
        /**
         * @brief Get LFO data
         * @return LFO data
         */
        const LFOData& getLFOData() const { return lfoData; }
        
        /**
         * @brief Get automation value at time
         * @param time Time in seconds
         * @param tempo Current tempo
         * @return Automation value
         */
        double getValueAtTime(double time, double tempo = 120.0) const;
        
        /**
         * @brief Process automation (call from audio thread)
         * @param numSamples Number of samples to process
         * @param sampleRate Sample rate
         * @param tempo Current tempo
         * @return Vector of automation values
         */
        std::vector<double> processAutomation(int numSamples, double sampleRate, double tempo = 120.0);
        
        /**
         * @brief Set time range
         * @param startTime Start time in seconds
         * @param endTime End time in seconds
         */
        void setTimeRange(double startTime, double endTime);
        
        /**
         * @brief Get time range
         * @return Pair of start and end times
         */
        std::pair<double, double> getTimeRange() const;
        
    private:
        std::string parameterId;
        AutomationType automationType;
        std::atomic<bool> enabled{true};
        
        // Keyframe data
        std::vector<AutomationPoint> keyframes;
        mutable std::mutex keyframeMutex;
        
        // LFO data
        LFOData lfoData;
        
        // Time range
        double startTime = 0.0;
        double endTime = 0.0;
        
        // Internal methods
        double interpolateKeyframes(double time) const;
        double calculateLFOValue(double time, double tempo) const;
        double calculateSineLFO(double time, double frequency, double phase) const;
        double calculateTriangleLFO(double time, double frequency, double phase) const;
        double calculateSquareLFO(double time, double frequency, double phase) const;
        double calculateSawtoothLFO(double time, double frequency, double phase) const;
        double calculateRandomLFO(double time) const;
    };
    
    /**
     * @class AutomationEngine
     * @brief High-performance automation engine
     * 
     * Provides sample-accurate automation with keyframe and LFO support,
     * real-time parameter binding, and smooth interpolation.
     */
    class AutomationEngine
    {
    public:
        /**
         * @brief Constructor
         * @param audioEngine Reference to audio engine
         */
        AutomationEngine(nomad::audio::AudioEngine& audioEngine);
        
        /**
         * @brief Destructor
         */
        ~AutomationEngine();
        
        /**
         * @brief Initialize the automation engine
         * @return true if successful
         */
        bool initialize();
        
        /**
         * @brief Shutdown the automation engine
         */
        void shutdown();
        
        /**
         * @brief Create automation lane
         * @param parameterId Parameter ID to automate
         * @return Lane ID if successful, -1 if failed
         */
        int createAutomationLane(const std::string& parameterId);
        
        /**
         * @brief Remove automation lane
         * @param laneId Lane ID
         * @return True if successful
         */
        bool removeAutomationLane(int laneId);
        
        /**
         * @brief Get automation lane
         * @param laneId Lane ID
         * @return Pointer to lane, nullptr if not found
         */
        AutomationLane* getAutomationLane(int laneId);
        
        /**
         * @brief Get all automation lanes
         * @return Vector of lane pointers
         */
        std::vector<AutomationLane*> getAutomationLanes();
        
        /**
         * @brief Set automation lane type
         * @param laneId Lane ID
         * @param type Automation type
         * @return True if successful
         */
        bool setAutomationLaneType(int laneId, AutomationType type);
        
        /**
         * @brief Enable/disable automation lane
         * @param laneId Lane ID
         * @param enabled True to enable
         * @return True if successful
         */
        bool setAutomationLaneEnabled(int laneId, bool enabled);
        
        /**
         * @brief Add keyframe to lane
         * @param laneId Lane ID
         * @param point Automation point
         * @return True if successful
         */
        bool addKeyframe(int laneId, const AutomationPoint& point);
        
        /**
         * @brief Remove keyframe from lane
         * @param laneId Lane ID
         * @param time Time of point to remove
         * @return True if successful
         */
        bool removeKeyframe(int laneId, double time);
        
        /**
         * @brief Clear keyframes from lane
         * @param laneId Lane ID
         * @return True if successful
         */
        bool clearKeyframes(int laneId);
        
        /**
         * @brief Set LFO data for lane
         * @param laneId Lane ID
         * @param lfoData LFO data
         * @return True if successful
         */
        bool setLFOData(int laneId, const LFOData& lfoData);
        
        /**
         * @brief Process automation (call from audio thread)
         * @param numSamples Number of samples to process
         * @param tempo Current tempo
         */
        void processAutomation(int numSamples, double tempo = 120.0);
        
        /**
         * @brief Set current time
         * @param time Time in seconds
         */
        void setCurrentTime(double time);
        
        /**
         * @brief Get current time
         * @return Time in seconds
         */
        double getCurrentTime() const { return currentTime.load(); }
        
        /**
         * @brief Set automation enabled
         * @param enabled True to enable automation
         */
        void setAutomationEnabled(bool enabled);
        
        /**
         * @brief Check if automation is enabled
         * @return True if enabled
         */
        bool isAutomationEnabled() const { return automationEnabled.load(); }
        
        /**
         * @brief Add automation callback
         * @param callback Function to call on automation changes
         */
        void addAutomationCallback(std::function<void(const std::string&, double)> callback);
        
        /**
         * @brief Remove automation callback
         * @param callback Function to remove
         */
        void removeAutomationCallback(std::function<void(const std::string&, double)> callback);
        
        /**
         * @brief Get automation value for parameter
         * @param parameterId Parameter ID
         * @return Automation value
         */
        double getAutomationValue(const std::string& parameterId) const;
        
        /**
         * @brief Export automation to XML
         * @return XML element containing automation data
         */
        juce::XmlElement* exportAutomationToXml() const;
        
        /**
         * @brief Import automation from XML
         * @param xml XML element containing automation data
         * @return True if successful
         */
        bool importAutomationFromXml(const juce::XmlElement& xml);
        
        /**
         * @brief Get automation statistics
         * @return Statistics structure
         */
        struct AutomationStats
        {
            int totalLanes = 0;
            int activeLanes = 0;
            int keyframeLanes = 0;
            int lfoLanes = 0;
            int totalKeyframes = 0;
            double averageLatency = 0.0;
        };
        
        AutomationStats getAutomationStats() const;
        
    private:
        // Reference to audio engine
        nomad::audio::AudioEngine& audioEngine;
        
        // Automation lanes
        std::unordered_map<int, std::unique_ptr<AutomationLane>> automationLanes;
        std::atomic<int> nextLaneId{1};
        
        // Current state
        std::atomic<double> currentTime{0.0};
        std::atomic<bool> automationEnabled{true};
        
        // Callbacks
        std::vector<std::function<void(const std::string&, double)>> automationCallbacks;
        std::mutex callbackMutex;
        
        // Statistics
        mutable std::atomic<int> totalLanes{0};
        mutable std::atomic<int> activeLanes{0};
        mutable std::atomic<int> keyframeLanes{0};
        mutable std::atomic<int> lfoLanes{0};
        mutable std::atomic<int> totalKeyframes{0};
        mutable std::atomic<double> averageLatency{0.0};
        
        // Internal methods
        void updateStatistics();
        void notifyAutomationChange(const std::string& parameterId, double value);
    };
}