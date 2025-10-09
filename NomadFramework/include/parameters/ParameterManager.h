/**
 * @file ParameterManager.h
 * @brief Thread-safe parameter system with smooth transitions
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
#include <string>

namespace nomad::parameters
{
    /**
     * @enum ParameterType
     * @brief Parameter type enumeration
     */
    enum class ParameterType
    {
        Float,
        Int,
        Bool,
        Choice,
        String
    };
    
    /**
     * @struct ParameterRange
     * @brief Parameter range definition
     */
    struct ParameterRange
    {
        double minValue = 0.0;
        double maxValue = 1.0;
        double defaultValue = 0.0;
        double stepSize = 0.01;
        
        ParameterRange() = default;
        ParameterRange(double min, double max, double def, double step = 0.01)
            : minValue(min), maxValue(max), defaultValue(def), stepSize(step) {}
    };
    
    /**
     * @class NomadParameter
     * @brief Generic parameter class with thread-safe communication
     * 
     * Provides smooth value transitions, value scaling, transformation support,
     * and thread-safe communication between UI and audio threads.
     */
    class NomadParameter : public juce::AudioProcessorParameter
    {
    public:
        /**
         * @brief Constructor
         * @param id Parameter ID
         * @param name Parameter name
         * @param type Parameter type
         * @param range Parameter range
         */
        NomadParameter(const std::string& id, const std::string& name, ParameterType type, const ParameterRange& range);
        
        /**
         * @brief Destructor
         */
        ~NomadParameter();
        
        // AudioProcessorParameter interface
        float getValue() const override;
        void setValue(float newValue) override;
        float getDefaultValue() const override;
        juce::String getName(int maximumStringLength) const override;
        juce::String getLabel() const override;
        int getNumSteps() const override;
        bool isDiscrete() const override;
        bool isBoolean() const override;
        juce::String getText(float value, int maximumStringLength) const override;
        float getValueForText(const juce::String& text) const override;
        bool isOrientationInverted() const override;
        bool isAutomatable() const override;
        bool isMetaParameter() const override;
        juce::AudioProcessorParameter::Category getCategory() const override;
        
        /**
         * @brief Get parameter ID
         * @return Parameter ID
         */
        const std::string& getParameterId() const { return parameterId; }
        
        /**
         * @brief Get parameter type
         * @return Parameter type
         */
        ParameterType getParameterType() const { return parameterType; }
        
        /**
         * @brief Get parameter range
         * @return Parameter range
         */
        const ParameterRange& getRange() const { return range; }
        
        /**
         * @brief Set parameter range
         * @param newRange New parameter range
         */
        void setRange(const ParameterRange& newRange);
        
        /**
         * @brief Get raw value (unscaled)
         * @return Raw value
         */
        double getRawValue() const;
        
        /**
         * @brief Set raw value (unscaled)
         * @param value Raw value
         */
        void setRawValue(double value);
        
        /**
         * @brief Get scaled value
         * @return Scaled value
         */
        double getScaledValue() const;
        
        /**
         * @brief Set scaled value
         * @param value Scaled value
         */
        void setScaledValue(double value);
        
        /**
         * @brief Set value with smooth transition
         * @param targetValue Target value
         * @param transitionTimeMs Transition time in milliseconds
         */
        void setValueSmooth(double targetValue, double transitionTimeMs = 100.0);
        
        /**
         * @brief Update parameter (call from audio thread)
         * @param numSamples Number of samples to process
         */
        void updateParameter(int numSamples);
        
        /**
         * @brief Add parameter change callback
         * @param callback Function to call on parameter changes
         */
        void addParameterCallback(std::function<void(double)> callback);
        
        /**
         * @brief Remove parameter change callback
         * @param callback Function to remove
         */
        void removeParameterCallback(std::function<void(double)> callback);
        
        /**
         * @brief Set value transformation function
         * @param transform Function to transform values
         */
        void setValueTransform(std::function<double(double)> transform);
        
        /**
         * @brief Set inverse value transformation function
         * @param inverseTransform Function to inverse transform values
         */
        void setInverseValueTransform(std::function<double(double)> inverseTransform);
        
        /**
         * @brief Get parameter value as string
         * @return Value as string
         */
        juce::String getValueAsString() const;
        
        /**
         * @brief Set parameter value from string
         * @param valueString Value as string
         * @return True if successful
         */
        bool setValueFromString(const juce::String& valueString);
        
    private:
        std::string parameterId;
        std::string parameterName;
        ParameterType parameterType;
        ParameterRange range;
        
        // Value management
        std::atomic<double> currentValue{0.0};
        std::atomic<double> targetValue{0.0};
        std::atomic<double> currentRawValue{0.0};
        std::atomic<double> targetRawValue{0.0};
        
        // Smooth transitions
        std::atomic<double> transitionRate{0.0};
        std::atomic<bool> isTransitioning{false};
        
        // Value transformation
        std::function<double(double)> valueTransform;
        std::function<double(double)> inverseValueTransform;
        
        // Callbacks
        std::vector<std::function<void(double)>> parameterCallbacks;
        std::mutex callbackMutex;
        
        // Internal methods
        double scaleValue(double rawValue) const;
        double unscaleValue(double scaledValue) const;
        void notifyCallbacks(double value);
    };
    
    /**
     * @class ParameterManager
     * @brief Centralized parameter management system
     * 
     * Manages all parameters, provides thread-safe access, and handles
     * parameter grouping and automation.
     */
    class ParameterManager
    {
    public:
        /**
         * @brief Constructor
         */
        ParameterManager();
        
        /**
         * @brief Destructor
         */
        ~ParameterManager();
        
        /**
         * @brief Initialize the parameter manager
         * @return true if successful
         */
        bool initialize();
        
        /**
         * @brief Shutdown the parameter manager
         */
        void shutdown();
        
        /**
         * @brief Create a new parameter
         * @param id Parameter ID
         * @param name Parameter name
         * @param type Parameter type
         * @param range Parameter range
         * @return Pointer to created parameter, nullptr if failed
         */
        NomadParameter* createParameter(const std::string& id, const std::string& name, 
                                      ParameterType type, const ParameterRange& range);
        
        /**
         * @brief Get parameter by ID
         * @param id Parameter ID
         * @return Pointer to parameter, nullptr if not found
         */
        NomadParameter* getParameter(const std::string& id);
        
        /**
         * @brief Get parameter by index
         * @param index Parameter index
         * @return Pointer to parameter, nullptr if not found
         */
        NomadParameter* getParameter(int index);
        
        /**
         * @brief Remove parameter
         * @param id Parameter ID
         * @return True if successful
         */
        bool removeParameter(const std::string& id);
        
        /**
         * @brief Get number of parameters
         * @return Number of parameters
         */
        int getNumParameters() const;
        
        /**
         * @brief Get all parameter IDs
         * @return Vector of parameter IDs
         */
        std::vector<std::string> getParameterIds() const;
        
        /**
         * @brief Set parameter value
         * @param id Parameter ID
         * @param value Parameter value
         * @return True if successful
         */
        bool setParameterValue(const std::string& id, double value);
        
        /**
         * @brief Get parameter value
         * @param id Parameter ID
         * @return Parameter value
         */
        double getParameterValue(const std::string& id) const;
        
        /**
         * @brief Set parameter value with smooth transition
         * @param id Parameter ID
         * @param value Target value
         * @param transitionTimeMs Transition time in milliseconds
         * @return True if successful
         */
        bool setParameterValueSmooth(const std::string& id, double value, double transitionTimeMs = 100.0);
        
        /**
         * @brief Update all parameters (call from audio thread)
         * @param numSamples Number of samples to process
         */
        void updateParameters(int numSamples);
        
        /**
         * @brief Add parameter group
         * @param groupName Group name
         * @param parameterIds Parameter IDs in group
         * @return True if successful
         */
        bool addParameterGroup(const std::string& groupName, const std::vector<std::string>& parameterIds);
        
        /**
         * @brief Get parameter group
         * @param groupName Group name
         * @return Vector of parameter IDs in group
         */
        std::vector<std::string> getParameterGroup(const std::string& groupName) const;
        
        /**
         * @brief Remove parameter group
         * @param groupName Group name
         * @return True if successful
         */
        bool removeParameterGroup(const std::string& groupName);
        
        /**
         * @brief Get all parameter groups
         * @return Map of group names to parameter IDs
         */
        std::unordered_map<std::string, std::vector<std::string>> getParameterGroups() const;
        
        /**
         * @brief Save parameters to XML
         * @return XML element containing parameters
         */
        juce::XmlElement* saveParametersToXml() const;
        
        /**
         * @brief Load parameters from XML
         * @param xml XML element containing parameters
         * @return True if successful
         */
        bool loadParametersFromXml(const juce::XmlElement& xml);
        
        /**
         * @brief Add parameter change callback
         * @param callback Function to call on parameter changes
         */
        void addParameterChangeCallback(std::function<void(const std::string&, double)> callback);
        
        /**
         * @brief Remove parameter change callback
         * @param callback Function to remove
         */
        void removeParameterChangeCallback(std::function<void(const std::string&, double)> callback);
        
        /**
         * @brief Get parameter statistics
         * @return Statistics structure
         */
        struct ParameterStats
        {
            int totalParameters = 0;
            int floatParameters = 0;
            int intParameters = 0;
            int boolParameters = 0;
            int choiceParameters = 0;
            int stringParameters = 0;
            int parameterGroups = 0;
        };
        
        ParameterStats getParameterStats() const;
        
    private:
        // Parameter storage
        std::unordered_map<std::string, std::unique_ptr<NomadParameter>> parameters;
        std::vector<std::string> parameterOrder;
        
        // Parameter groups
        std::unordered_map<std::string, std::vector<std::string>> parameterGroups;
        
        // Threading
        mutable std::mutex parameterMutex;
        std::vector<std::function<void(const std::string&, double)>> parameterChangeCallbacks;
        std::mutex callbackMutex;
        
        // Statistics
        mutable std::atomic<int> totalParameters{0};
        mutable std::atomic<int> floatParameters{0};
        mutable std::atomic<int> intParameters{0};
        mutable std::atomic<int> boolParameters{0};
        mutable std::atomic<int> choiceParameters{0};
        mutable std::atomic<int> stringParameters{0};
        
        // Internal methods
        void updateStatistics();
        void notifyParameterChange(const std::string& id, double value);
    };
}