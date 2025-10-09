/**
 * @file AutomationEngine.cpp
 * @brief Sample-accurate automation engine implementation
 */

#include "automation/AutomationEngine.h"
#include <algorithm>
#include <cmath>
#include <random>

namespace nomad::automation
{
    // AutomationLane implementation
    AutomationLane::AutomationLane(const std::string& parameterId)
        : parameterId(parameterId), automationType(AutomationType::Keyframe)
    {
    }
    
    AutomationLane::~AutomationLane()
    {
    }
    
    void AutomationLane::setAutomationType(AutomationType type)
    {
        automationType = type;
    }
    
    void AutomationLane::setEnabled(bool enabled)
    {
        this->enabled = enabled;
    }
    
    void AutomationLane::addKeyframe(const AutomationPoint& point)
    {
        std::lock_guard<std::mutex> lock(keyframeMutex);
        
        // Insert keyframe in time order
        auto it = std::lower_bound(keyframes.begin(), keyframes.end(), point,
            [](const AutomationPoint& a, const AutomationPoint& b) {
                return a.time < b.time;
            });
        
        keyframes.insert(it, point);
    }
    
    bool AutomationLane::removeKeyframe(double time)
    {
        std::lock_guard<std::mutex> lock(keyframeMutex);
        
        auto it = std::find_if(keyframes.begin(), keyframes.end(),
            [time](const AutomationPoint& point) {
                return std::abs(point.time - time) < 0.001;
            });
        
        if (it != keyframes.end())
        {
            keyframes.erase(it);
            return true;
        }
        
        return false;
    }
    
    void AutomationLane::clearKeyframes()
    {
        std::lock_guard<std::mutex> lock(keyframeMutex);
        keyframes.clear();
    }
    
    const AutomationPoint* AutomationLane::getKeyframeAtTime(double time) const
    {
        std::lock_guard<std::mutex> lock(keyframeMutex);
        
        auto it = std::find_if(keyframes.begin(), keyframes.end(),
            [time](const AutomationPoint& point) {
                return std::abs(point.time - time) < 0.001;
            });
        
        if (it != keyframes.end())
        {
            return &(*it);
        }
        
        return nullptr;
    }
    
    std::vector<AutomationPoint> AutomationLane::getKeyframes() const
    {
        std::lock_guard<std::mutex> lock(keyframeMutex);
        return keyframes;
    }
    
    void AutomationLane::setLFOData(const LFOData& data)
    {
        lfoData = data;
    }
    
    double AutomationLane::getValueAtTime(double time, double tempo) const
    {
        if (!enabled.load())
            return 0.0;
        
        switch (automationType)
        {
            case AutomationType::Keyframe:
                return interpolateKeyframes(time);
            case AutomationType::LFO:
                return calculateLFOValue(time, tempo);
            case AutomationType::Envelope:
                return interpolateKeyframes(time); // Envelopes use keyframes
            case AutomationType::Curve:
                return interpolateKeyframes(time); // Curves use keyframes
            default:
                return 0.0;
        }
    }
    
    std::vector<double> AutomationLane::processAutomation(int numSamples, double sampleRate, double tempo)
    {
        std::vector<double> values(numSamples);
        
        if (!enabled.load())
        {
            std::fill(values.begin(), values.end(), 0.0);
            return values;
        }
        
        double timeIncrement = 1.0 / sampleRate;
        double currentTime = this->currentTime.load();
        
        for (int i = 0; i < numSamples; ++i)
        {
            values[i] = getValueAtTime(currentTime, tempo);
            currentTime += timeIncrement;
        }
        
        return values;
    }
    
    void AutomationLane::setTimeRange(double start, double end)
    {
        startTime = start;
        endTime = end;
    }
    
    std::pair<double, double> AutomationLane::getTimeRange() const
    {
        return {startTime, endTime};
    }
    
    double AutomationLane::interpolateKeyframes(double time) const
    {
        std::lock_guard<std::mutex> lock(keyframeMutex);
        
        if (keyframes.empty())
            return 0.0;
        
        // Find surrounding keyframes
        auto it = std::lower_bound(keyframes.begin(), keyframes.end(), time,
            [](const AutomationPoint& point, double t) {
                return point.time < t;
            });
        
        if (it == keyframes.begin())
        {
            return it->value;
        }
        
        if (it == keyframes.end())
        {
            return keyframes.back().value;
        }
        
        // Interpolate between keyframes
        const AutomationPoint& prev = *(it - 1);
        const AutomationPoint& next = *it;
        
        double t = (time - prev.time) / (next.time - prev.time);
        t = juce::jlimit(0.0, 1.0, t);
        
        // Apply curve interpolation
        double curve = prev.curve;
        if (curve > 0.0)
        {
            t = std::pow(t, 1.0 + curve);
        }
        else if (curve < 0.0)
        {
            t = 1.0 - std::pow(1.0 - t, 1.0 - curve);
        }
        
        return prev.value + (next.value - prev.value) * t;
    }
    
    double AutomationLane::calculateLFOValue(double time, double tempo) const
    {
        double frequency = lfoData.frequency;
        
        if (lfoData.syncToTempo)
        {
            frequency = (tempo / 60.0) * lfoData.tempoMultiplier;
        }
        
        double phase = lfoData.phase + time * frequency;
        phase = std::fmod(phase, 1.0);
        
        double value = 0.0;
        switch (lfoData.type)
        {
            case LFOType::Sine:
                value = calculateSineLFO(time, frequency, lfoData.phase);
                break;
            case LFOType::Triangle:
                value = calculateTriangleLFO(time, frequency, lfoData.phase);
                break;
            case LFOType::Square:
                value = calculateSquareLFO(time, frequency, lfoData.phase);
                break;
            case LFOType::Sawtooth:
                value = calculateSawtoothLFO(time, frequency, lfoData.phase);
                break;
            case LFOType::Random:
                value = calculateRandomLFO(time);
                break;
        }
        
        return lfoData.offset + value * lfoData.amplitude;
    }
    
    double AutomationLane::calculateSineLFO(double time, double frequency, double phase) const
    {
        return std::sin(2.0 * juce::MathConstants<double>::pi * (time * frequency + phase));
    }
    
    double AutomationLane::calculateTriangleLFO(double time, double frequency, double phase) const
    {
        double t = std::fmod(time * frequency + phase, 1.0);
        if (t < 0.5)
            return 4.0 * t - 1.0;
        else
            return 3.0 - 4.0 * t;
    }
    
    double AutomationLane::calculateSquareLFO(double time, double frequency, double phase) const
    {
        double t = std::fmod(time * frequency + phase, 1.0);
        return t < 0.5 ? 1.0 : -1.0;
    }
    
    double AutomationLane::calculateSawtoothLFO(double time, double frequency, double phase) const
    {
        double t = std::fmod(time * frequency + phase, 1.0);
        return 2.0 * t - 1.0;
    }
    
    double AutomationLane::calculateRandomLFO(double time) const
    {
        static std::random_device rd;
        static std::mt19937 gen(rd());
        static std::uniform_real_distribution<double> dis(-1.0, 1.0);
        
        // Use time as seed for deterministic randomness
        gen.seed(static_cast<unsigned int>(time * 1000.0));
        return dis(gen);
    }
    
    // AutomationEngine implementation
    AutomationEngine::AutomationEngine(nomad::audio::AudioEngine& audioEngine)
        : audioEngine(audioEngine)
    {
    }
    
    AutomationEngine::~AutomationEngine()
    {
        shutdown();
    }
    
    bool AutomationEngine::initialize()
    {
        try
        {
            currentTime = 0.0;
            automationEnabled = true;
            return true;
        }
        catch (const std::exception& e)
        {
            return false;
        }
    }
    
    void AutomationEngine::shutdown()
    {
        std::lock_guard<std::mutex> lock(callbackMutex);
        automationLanes.clear();
        automationCallbacks.clear();
    }
    
    int AutomationEngine::createAutomationLane(const std::string& parameterId)
    {
        int laneId = nextLaneId++;
        automationLanes[laneId] = std::make_unique<AutomationLane>(parameterId);
        updateStatistics();
        return laneId;
    }
    
    bool AutomationEngine::removeAutomationLane(int laneId)
    {
        auto it = automationLanes.find(laneId);
        if (it != automationLanes.end())
        {
            automationLanes.erase(it);
            updateStatistics();
            return true;
        }
        return false;
    }
    
    AutomationLane* AutomationEngine::getAutomationLane(int laneId)
    {
        auto it = automationLanes.find(laneId);
        if (it != automationLanes.end())
        {
            return it->second.get();
        }
        return nullptr;
    }
    
    std::vector<AutomationLane*> AutomationEngine::getAutomationLanes()
    {
        std::vector<AutomationLane*> lanes;
        for (auto& pair : automationLanes)
        {
            lanes.push_back(pair.second.get());
        }
        return lanes;
    }
    
    bool AutomationEngine::setAutomationLaneType(int laneId, AutomationType type)
    {
        auto* lane = getAutomationLane(laneId);
        if (lane)
        {
            lane->setAutomationType(type);
            updateStatistics();
            return true;
        }
        return false;
    }
    
    bool AutomationEngine::setAutomationLaneEnabled(int laneId, bool enabled)
    {
        auto* lane = getAutomationLane(laneId);
        if (lane)
        {
            lane->setEnabled(enabled);
            updateStatistics();
            return true;
        }
        return false;
    }
    
    bool AutomationEngine::addKeyframe(int laneId, const AutomationPoint& point)
    {
        auto* lane = getAutomationLane(laneId);
        if (lane)
        {
            lane->addKeyframe(point);
            updateStatistics();
            return true;
        }
        return false;
    }
    
    bool AutomationEngine::removeKeyframe(int laneId, double time)
    {
        auto* lane = getAutomationLane(laneId);
        if (lane)
        {
            bool removed = lane->removeKeyframe(time);
            if (removed)
            {
                updateStatistics();
            }
            return removed;
        }
        return false;
    }
    
    bool AutomationEngine::clearKeyframes(int laneId)
    {
        auto* lane = getAutomationLane(laneId);
        if (lane)
        {
            lane->clearKeyframes();
            updateStatistics();
            return true;
        }
        return false;
    }
    
    bool AutomationEngine::setLFOData(int laneId, const LFOData& lfoData)
    {
        auto* lane = getAutomationLane(laneId);
        if (lane)
        {
            lane->setLFOData(lfoData);
            return true;
        }
        return false;
    }
    
    void AutomationEngine::processAutomation(int numSamples, double tempo)
    {
        if (!automationEnabled.load())
            return;
        
        double sampleRate = audioEngine.getSampleRate();
        double timeIncrement = 1.0 / sampleRate;
        
        for (auto& pair : automationLanes)
        {
            auto* lane = pair.second.get();
            if (lane && lane->isEnabled())
            {
                auto values = lane->processAutomation(numSamples, sampleRate, tempo);
                
                // Apply automation values to parameters
                for (int i = 0; i < numSamples; ++i)
                {
                    double value = values[i];
                    notifyAutomationChange(lane->getParameterId(), value);
                }
            }
        }
        
        // Update current time
        currentTime = currentTime.load() + numSamples * timeIncrement;
    }
    
    void AutomationEngine::setCurrentTime(double time)
    {
        currentTime = time;
    }
    
    void AutomationEngine::setAutomationEnabled(bool enabled)
    {
        automationEnabled = enabled;
    }
    
    void AutomationEngine::addAutomationCallback(std::function<void(const std::string&, double)> callback)
    {
        std::lock_guard<std::mutex> lock(callbackMutex);
        automationCallbacks.push_back(callback);
    }
    
    void AutomationEngine::removeAutomationCallback(std::function<void(const std::string&, double)> callback)
    {
        std::lock_guard<std::mutex> lock(callbackMutex);
        automationCallbacks.erase(
            std::remove(automationCallbacks.begin(), automationCallbacks.end(), callback),
            automationCallbacks.end()
        );
    }
    
    double AutomationEngine::getAutomationValue(const std::string& parameterId) const
    {
        for (auto& pair : automationLanes)
        {
            auto* lane = pair.second.get();
            if (lane && lane->getParameterId() == parameterId && lane->isEnabled())
            {
                return lane->getValueAtTime(currentTime.load());
            }
        }
        return 0.0;
    }
    
    juce::XmlElement* AutomationEngine::exportAutomationToXml() const
    {
        auto* xml = new juce::XmlElement("Automation");
        
        for (auto& pair : automationLanes)
        {
            auto* lane = pair.second.get();
            if (lane)
            {
                auto* laneXml = new juce::XmlElement("Lane");
                laneXml->setAttribute("id", pair.first);
                laneXml->setAttribute("parameterId", lane->getParameterId());
                laneXml->setAttribute("type", static_cast<int>(lane->getAutomationType()));
                laneXml->setAttribute("enabled", lane->isEnabled());
                
                // Export keyframes
                auto keyframes = lane->getKeyframes();
                for (const auto& point : keyframes)
                {
                    auto* pointXml = new juce::XmlElement("Keyframe");
                    pointXml->setAttribute("time", point.time);
                    pointXml->setAttribute("value", point.value);
                    pointXml->setAttribute("curve", point.curve);
                    laneXml->addChildElement(pointXml);
                }
                
                xml->addChildElement(laneXml);
            }
        }
        
        return xml;
    }
    
    bool AutomationEngine::importAutomationFromXml(const juce::XmlElement& xml)
    {
        if (xml.getTagName() != "Automation")
            return false;
        
        for (auto* laneXml : xml.getChildIterator())
        {
            if (laneXml->getTagName() == "Lane")
            {
                int laneId = laneXml->getIntAttribute("id");
                std::string parameterId = laneXml->getStringAttribute("parameterId").toStdString();
                AutomationType type = static_cast<AutomationType>(laneXml->getIntAttribute("type"));
                bool enabled = laneXml->getBoolAttribute("enabled");
                
                // Create lane
                automationLanes[laneId] = std::make_unique<AutomationLane>(parameterId);
                auto* lane = automationLanes[laneId].get();
                lane->setAutomationType(type);
                lane->setEnabled(enabled);
                
                // Import keyframes
                for (auto* pointXml : laneXml->getChildIterator())
                {
                    if (pointXml->getTagName() == "Keyframe")
                    {
                        AutomationPoint point;
                        point.time = pointXml->getDoubleAttribute("time");
                        point.value = pointXml->getDoubleAttribute("value");
                        point.curve = pointXml->getDoubleAttribute("curve");
                        lane->addKeyframe(point);
                    }
                }
            }
        }
        
        updateStatistics();
        return true;
    }
    
    AutomationEngine::AutomationStats AutomationEngine::getAutomationStats() const
    {
        AutomationStats stats;
        stats.totalLanes = totalLanes.load();
        stats.activeLanes = activeLanes.load();
        stats.keyframeLanes = keyframeLanes.load();
        stats.lfoLanes = lfoLanes.load();
        stats.totalKeyframes = totalKeyframes.load();
        stats.averageLatency = averageLatency.load();
        return stats;
    }
    
    void AutomationEngine::updateStatistics()
    {
        int total = 0;
        int active = 0;
        int keyframe = 0;
        int lfo = 0;
        int keyframes = 0;
        
        for (auto& pair : automationLanes)
        {
            auto* lane = pair.second.get();
            if (lane)
            {
                total++;
                if (lane->isEnabled())
                    active++;
                
                if (lane->getAutomationType() == AutomationType::Keyframe)
                    keyframe++;
                else if (lane->getAutomationType() == AutomationType::LFO)
                    lfo++;
                
                keyframes += static_cast<int>(lane->getKeyframes().size());
            }
        }
        
        totalLanes = total;
        activeLanes = active;
        keyframeLanes = keyframe;
        lfoLanes = lfo;
        totalKeyframes = keyframes;
    }
    
    void AutomationEngine::notifyAutomationChange(const std::string& parameterId, double value)
    {
        std::lock_guard<std::mutex> lock(callbackMutex);
        for (auto& callback : automationCallbacks)
        {
            callback(parameterId, value);
        }
    }
}