/**
 * @file ParameterManager.cpp
 * @brief Thread-safe parameter system implementation
 */

#include "parameters/ParameterManager.h"
#include <algorithm>
#include <cmath>

namespace nomad::parameters
{
    // NomadParameter implementation
    NomadParameter::NomadParameter(const std::string& id, const std::string& name, 
                                  ParameterType type, const ParameterRange& range)
        : parameterId(id), parameterName(name), parameterType(type), range(range)
    {
        currentValue = range.defaultValue;
        targetValue = range.defaultValue;
        currentRawValue = range.defaultValue;
        targetRawValue = range.defaultValue;
    }
    
    NomadParameter::~NomadParameter()
    {
    }
    
    float NomadParameter::getValue() const
    {
        return static_cast<float>(currentValue.load());
    }
    
    void NomadParameter::setValue(float newValue)
    {
        setScaledValue(static_cast<double>(newValue));
    }
    
    float NomadParameter::getDefaultValue() const
    {
        return static_cast<float>(range.defaultValue);
    }
    
    juce::String NomadParameter::getName(int maximumStringLength) const
    {
        juce::String name(parameterName);
        if (maximumStringLength > 0 && name.length() > maximumStringLength)
        {
            name = name.substring(0, maximumStringLength);
        }
        return name;
    }
    
    juce::String NomadParameter::getLabel() const
    {
        switch (parameterType)
        {
            case ParameterType::Float:
                return "float";
            case ParameterType::Int:
                return "int";
            case ParameterType::Bool:
                return "bool";
            case ParameterType::Choice:
                return "choice";
            case ParameterType::String:
                return "string";
            default:
                return "unknown";
        }
    }
    
    int NomadParameter::getNumSteps() const
    {
        if (parameterType == ParameterType::Bool)
            return 2;
        
        if (parameterType == ParameterType::Int)
        {
            return static_cast<int>(range.maxValue - range.minValue) + 1;
        }
        
        if (range.stepSize > 0.0)
        {
            return static_cast<int>((range.maxValue - range.minValue) / range.stepSize) + 1;
        }
        
        return juce::AudioProcessorParameter::getNumSteps();
    }
    
    bool NomadParameter::isDiscrete() const
    {
        return parameterType == ParameterType::Int || parameterType == ParameterType::Bool || parameterType == ParameterType::Choice;
    }
    
    bool NomadParameter::isBoolean() const
    {
        return parameterType == ParameterType::Bool;
    }
    
    juce::String NomadParameter::getText(float value, int maximumStringLength) const
    {
        double scaledValue = scaleValue(static_cast<double>(value));
        juce::String text;
        
        switch (parameterType)
        {
            case ParameterType::Float:
                text = juce::String(scaledValue, 3);
                break;
            case ParameterType::Int:
                text = juce::String(static_cast<int>(scaledValue));
                break;
            case ParameterType::Bool:
                text = scaledValue > 0.5 ? "On" : "Off";
                break;
            case ParameterType::Choice:
                text = juce::String(static_cast<int>(scaledValue));
                break;
            case ParameterType::String:
                text = juce::String(scaledValue);
                break;
        }
        
        if (maximumStringLength > 0 && text.length() > maximumStringLength)
        {
            text = text.substring(0, maximumStringLength);
        }
        
        return text;
    }
    
    float NomadParameter::getValueForText(const juce::String& text) const
    {
        double value = 0.0;
        
        switch (parameterType)
        {
            case ParameterType::Float:
                value = text.getDoubleValue();
                break;
            case ParameterType::Int:
                value = static_cast<double>(text.getIntValue());
                break;
            case ParameterType::Bool:
                value = text.toLowerCase() == "on" || text == "1" ? 1.0 : 0.0;
                break;
            case ParameterType::Choice:
                value = static_cast<double>(text.getIntValue());
                break;
            case ParameterType::String:
                value = text.getDoubleValue();
                break;
        }
        
        return static_cast<float>(unscaleValue(value));
    }
    
    bool NomadParameter::isOrientationInverted() const
    {
        return false;
    }
    
    bool NomadParameter::isAutomatable() const
    {
        return true;
    }
    
    bool NomadParameter::isMetaParameter() const
    {
        return false;
    }
    
    juce::AudioProcessorParameter::Category NomadParameter::getCategory() const
    {
        return juce::AudioProcessorParameter::genericParameter;
    }
    
    void NomadParameter::setRange(const ParameterRange& newRange)
    {
        range = newRange;
    }
    
    double NomadParameter::getRawValue() const
    {
        return currentRawValue.load();
    }
    
    void NomadParameter::setRawValue(double value)
    {
        value = juce::jlimit(range.minValue, range.maxValue, value);
        currentRawValue = value;
        currentValue = scaleValue(value);
        notifyCallbacks(currentValue);
    }
    
    double NomadParameter::getScaledValue() const
    {
        return currentValue.load();
    }
    
    void NomadParameter::setScaledValue(double value)
    {
        value = juce::jlimit(0.0, 1.0, value);
        currentValue = value;
        currentRawValue = unscaleValue(value);
        notifyCallbacks(currentValue);
    }
    
    void NomadParameter::setValueSmooth(double targetValue, double transitionTimeMs)
    {
        this->targetValue = juce::jlimit(0.0, 1.0, targetValue);
        this->targetRawValue = unscaleValue(this->targetValue.load());
        
        if (transitionTimeMs > 0.0)
        {
            // Calculate transition rate based on sample rate
            double sampleRate = 44100.0; // This should be passed from audio engine
            double transitionSamples = (transitionTimeMs / 1000.0) * sampleRate;
            transitionRate = 1.0 / transitionSamples;
            isTransitioning = true;
        }
        else
        {
            currentValue = this->targetValue.load();
            currentRawValue = this->targetRawValue.load();
            isTransitioning = false;
        }
    }
    
    void NomadParameter::updateParameter(int numSamples)
    {
        if (!isTransitioning.load())
            return;
        
        double current = currentValue.load();
        double target = targetValue.load();
        double rate = transitionRate.load();
        
        if (std::abs(target - current) < 0.001)
        {
            currentValue = target;
            currentRawValue = targetRawValue.load();
            isTransitioning = false;
            notifyCallbacks(currentValue);
        }
        else
        {
            double newValue = current + (target - current) * rate * numSamples;
            currentValue = newValue;
            currentRawValue = unscaleValue(newValue);
            notifyCallbacks(currentValue);
        }
    }
    
    void NomadParameter::addParameterCallback(std::function<void(double)> callback)
    {
        std::lock_guard<std::mutex> lock(callbackMutex);
        parameterCallbacks.push_back(callback);
    }
    
    void NomadParameter::removeParameterCallback(std::function<void(double)> callback)
    {
        std::lock_guard<std::mutex> lock(callbackMutex);
        parameterCallbacks.erase(
            std::remove(parameterCallbacks.begin(), parameterCallbacks.end(), callback),
            parameterCallbacks.end()
        );
    }
    
    void NomadParameter::setValueTransform(std::function<double(double)> transform)
    {
        valueTransform = transform;
    }
    
    void NomadParameter::setInverseValueTransform(std::function<double(double)> inverseTransform)
    {
        inverseValueTransform = inverseTransform;
    }
    
    juce::String NomadParameter::getValueAsString() const
    {
        return getText(static_cast<float>(currentValue.load()), 0);
    }
    
    bool NomadParameter::setValueFromString(const juce::String& valueString)
    {
        double value = valueString.getDoubleValue();
        if (valueString.isNotEmpty())
        {
            setScaledValue(value);
            return true;
        }
        return false;
    }
    
    double NomadParameter::scaleValue(double rawValue) const
    {
        double scaled = (rawValue - range.minValue) / (range.maxValue - range.minValue);
        scaled = juce::jlimit(0.0, 1.0, scaled);
        
        if (valueTransform)
        {
            scaled = valueTransform(scaled);
        }
        
        return scaled;
    }
    
    double NomadParameter::unscaleValue(double scaledValue) const
    {
        double raw = scaledValue;
        
        if (inverseValueTransform)
        {
            raw = inverseValueTransform(raw);
        }
        
        raw = range.minValue + raw * (range.maxValue - range.minValue);
        return juce::jlimit(range.minValue, range.maxValue, raw);
    }
    
    void NomadParameter::notifyCallbacks(double value)
    {
        std::lock_guard<std::mutex> lock(callbackMutex);
        for (auto& callback : parameterCallbacks)
        {
            callback(value);
        }
    }
    
    // ParameterManager implementation
    ParameterManager::ParameterManager()
    {
    }
    
    ParameterManager::~ParameterManager()
    {
        shutdown();
    }
    
    bool ParameterManager::initialize()
    {
        try
        {
            // Initialize parameter storage
            parameters.clear();
            parameterOrder.clear();
            parameterGroups.clear();
            
            return true;
        }
        catch (const std::exception& e)
        {
            return false;
        }
    }
    
    void ParameterManager::shutdown()
    {
        std::lock_guard<std::mutex> lock(parameterMutex);
        parameters.clear();
        parameterOrder.clear();
        parameterGroups.clear();
    }
    
    NomadParameter* ParameterManager::createParameter(const std::string& id, const std::string& name, 
                                                     ParameterType type, const ParameterRange& range)
    {
        std::lock_guard<std::mutex> lock(parameterMutex);
        
        // Check if parameter already exists
        if (parameters.find(id) != parameters.end())
        {
            return nullptr;
        }
        
        // Create new parameter
        auto parameter = std::make_unique<NomadParameter>(id, name, type, range);
        NomadParameter* paramPtr = parameter.get();
        
        parameters[id] = std::move(parameter);
        parameterOrder.push_back(id);
        
        updateStatistics();
        return paramPtr;
    }
    
    NomadParameter* ParameterManager::getParameter(const std::string& id)
    {
        std::lock_guard<std::mutex> lock(parameterMutex);
        
        auto it = parameters.find(id);
        if (it != parameters.end())
        {
            return it->second.get();
        }
        
        return nullptr;
    }
    
    NomadParameter* ParameterManager::getParameter(int index)
    {
        std::lock_guard<std::mutex> lock(parameterMutex);
        
        if (index >= 0 && index < static_cast<int>(parameterOrder.size()))
        {
            const std::string& id = parameterOrder[index];
            auto it = parameters.find(id);
            if (it != parameters.end())
            {
                return it->second.get();
            }
        }
        
        return nullptr;
    }
    
    bool ParameterManager::removeParameter(const std::string& id)
    {
        std::lock_guard<std::mutex> lock(parameterMutex);
        
        auto it = parameters.find(id);
        if (it != parameters.end())
        {
            parameters.erase(it);
            parameterOrder.erase(
                std::remove(parameterOrder.begin(), parameterOrder.end(), id),
                parameterOrder.end()
            );
            
            updateStatistics();
            return true;
        }
        
        return false;
    }
    
    int ParameterManager::getNumParameters() const
    {
        std::lock_guard<std::mutex> lock(parameterMutex);
        return static_cast<int>(parameters.size());
    }
    
    std::vector<std::string> ParameterManager::getParameterIds() const
    {
        std::lock_guard<std::mutex> lock(parameterMutex);
        return parameterOrder;
    }
    
    bool ParameterManager::setParameterValue(const std::string& id, double value)
    {
        auto* parameter = getParameter(id);
        if (parameter)
        {
            parameter->setScaledValue(value);
            notifyParameterChange(id, value);
            return true;
        }
        return false;
    }
    
    double ParameterManager::getParameterValue(const std::string& id) const
    {
        auto* parameter = const_cast<ParameterManager*>(this)->getParameter(id);
        if (parameter)
        {
            return parameter->getScaledValue();
        }
        return 0.0;
    }
    
    bool ParameterManager::setParameterValueSmooth(const std::string& id, double value, double transitionTimeMs)
    {
        auto* parameter = getParameter(id);
        if (parameter)
        {
            parameter->setValueSmooth(value, transitionTimeMs);
            notifyParameterChange(id, value);
            return true;
        }
        return false;
    }
    
    void ParameterManager::updateParameters(int numSamples)
    {
        std::lock_guard<std::mutex> lock(parameterMutex);
        
        for (auto& pair : parameters)
        {
            pair.second->updateParameter(numSamples);
        }
    }
    
    bool ParameterManager::addParameterGroup(const std::string& groupName, const std::vector<std::string>& parameterIds)
    {
        std::lock_guard<std::mutex> lock(parameterMutex);
        
        // Validate that all parameters exist
        for (const auto& id : parameterIds)
        {
            if (parameters.find(id) == parameters.end())
            {
                return false;
            }
        }
        
        parameterGroups[groupName] = parameterIds;
        return true;
    }
    
    std::vector<std::string> ParameterManager::getParameterGroup(const std::string& groupName) const
    {
        std::lock_guard<std::mutex> lock(parameterMutex);
        
        auto it = parameterGroups.find(groupName);
        if (it != parameterGroups.end())
        {
            return it->second;
        }
        
        return std::vector<std::string>();
    }
    
    bool ParameterManager::removeParameterGroup(const std::string& groupName)
    {
        std::lock_guard<std::mutex> lock(parameterMutex);
        
        auto it = parameterGroups.find(groupName);
        if (it != parameterGroups.end())
        {
            parameterGroups.erase(it);
            return true;
        }
        
        return false;
    }
    
    std::unordered_map<std::string, std::vector<std::string>> ParameterManager::getParameterGroups() const
    {
        std::lock_guard<std::mutex> lock(parameterMutex);
        return parameterGroups;
    }
    
    juce::XmlElement* ParameterManager::saveParametersToXml() const
    {
        auto* xml = new juce::XmlElement("Parameters");
        
        std::lock_guard<std::mutex> lock(parameterMutex);
        
        for (const auto& pair : parameters)
        {
            auto* paramXml = new juce::XmlElement("Parameter");
            paramXml->setAttribute("id", pair.first);
            paramXml->setAttribute("name", pair.second->getName(0));
            paramXml->setAttribute("value", pair.second->getScaledValue());
            paramXml->setAttribute("type", static_cast<int>(pair.second->getParameterType()));
            xml->addChildElement(paramXml);
        }
        
        return xml;
    }
    
    bool ParameterManager::loadParametersFromXml(const juce::XmlElement& xml)
    {
        if (xml.getTagName() != "Parameters")
            return false;
        
        std::lock_guard<std::mutex> lock(parameterMutex);
        
        for (auto* paramXml : xml.getChildIterator())
        {
            if (paramXml->getTagName() == "Parameter")
            {
                std::string id = paramXml->getStringAttribute("id").toStdString();
                auto it = parameters.find(id);
                if (it != parameters.end())
                {
                    double value = paramXml->getDoubleAttribute("value");
                    it->second->setScaledValue(value);
                }
            }
        }
        
        return true;
    }
    
    void ParameterManager::addParameterChangeCallback(std::function<void(const std::string&, double)> callback)
    {
        std::lock_guard<std::mutex> lock(callbackMutex);
        parameterChangeCallbacks.push_back(callback);
    }
    
    void ParameterManager::removeParameterChangeCallback(std::function<void(const std::string&, double)> callback)
    {
        std::lock_guard<std::mutex> lock(callbackMutex);
        parameterChangeCallbacks.erase(
            std::remove(parameterChangeCallbacks.begin(), parameterChangeCallbacks.end(), callback),
            parameterChangeCallbacks.end()
        );
    }
    
    ParameterManager::ParameterStats ParameterManager::getParameterStats() const
    {
        ParameterStats stats;
        stats.totalParameters = totalParameters.load();
        stats.floatParameters = floatParameters.load();
        stats.intParameters = intParameters.load();
        stats.boolParameters = boolParameters.load();
        stats.choiceParameters = choiceParameters.load();
        stats.stringParameters = stringParameters.load();
        stats.parameterGroups = static_cast<int>(parameterGroups.size());
        return stats;
    }
    
    void ParameterManager::updateStatistics()
    {
        int total = 0;
        int floatCount = 0;
        int intCount = 0;
        int boolCount = 0;
        int choiceCount = 0;
        int stringCount = 0;
        
        for (const auto& pair : parameters)
        {
            total++;
            switch (pair.second->getParameterType())
            {
                case ParameterType::Float:
                    floatCount++;
                    break;
                case ParameterType::Int:
                    intCount++;
                    break;
                case ParameterType::Bool:
                    boolCount++;
                    break;
                case ParameterType::Choice:
                    choiceCount++;
                    break;
                case ParameterType::String:
                    stringCount++;
                    break;
            }
        }
        
        totalParameters = total;
        floatParameters = floatCount;
        intParameters = intCount;
        boolParameters = boolCount;
        choiceParameters = choiceCount;
        stringParameters = stringCount;
    }
    
    void ParameterManager::notifyParameterChange(const std::string& id, double value)
    {
        std::lock_guard<std::mutex> lock(callbackMutex);
        for (auto& callback : parameterChangeCallbacks)
        {
            callback(id, value);
        }
    }
}