/**
 * @file PluginHost.h
 * @brief VST3/AU plugin host with safe loading and parameter management
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

namespace nomad::plugins
{
    /**
     * @struct PluginInfo
     * @brief Plugin information structure
     */
    struct PluginInfo
    {
        juce::String name;
        juce::String manufacturer;
        juce::String version;
        juce::String category;
        juce::String filePath;
        juce::PluginDescription description;
        bool isVST3 = false;
        bool isAU = false;
        bool isLoaded = false;
    };
    
    /**
     * @class PluginInstance
     * @brief Wrapper for plugin instances with safety features
     */
    class PluginInstance
    {
    public:
        /**
         * @brief Constructor
         * @param processor The audio processor
         * @param info Plugin information
         */
        PluginInstance(std::unique_ptr<juce::AudioPluginInstance> processor, const PluginInfo& info);
        
        /**
         * @brief Destructor
         */
        ~PluginInstance();
        
        /**
         * @brief Get the audio processor
         * @return Reference to the processor
         */
        juce::AudioPluginInstance& getProcessor() { return *processor; }
        
        /**
         * @brief Get plugin information
         * @return Plugin information
         */
        const PluginInfo& getInfo() const { return info; }
        
        /**
         * @brief Check if plugin is loaded
         * @return True if loaded
         */
        bool isLoaded() const { return info.isLoaded; }
        
        /**
         * @brief Load plugin
         * @param sampleRate Sample rate
         * @param bufferSize Buffer size
         * @return True if successful
         */
        bool load(double sampleRate, int bufferSize);
        
        /**
         * @brief Unload plugin
         */
        void unload();
        
        /**
         * @brief Get parameter value
         * @param parameterIndex Parameter index
         * @return Parameter value (0.0 to 1.0)
         */
        float getParameterValue(int parameterIndex) const;
        
        /**
         * @brief Set parameter value
         * @param parameterIndex Parameter index
         * @param value Parameter value (0.0 to 1.0)
         */
        void setParameterValue(int parameterIndex, float value);
        
        /**
         * @brief Get parameter name
         * @param parameterIndex Parameter index
         * @return Parameter name
         */
        juce::String getParameterName(int parameterIndex) const;
        
        /**
         * @brief Get parameter text
         * @param parameterIndex Parameter index
         * @param value Parameter value
         * @return Parameter text representation
         */
        juce::String getParameterText(int parameterIndex, float value) const;
        
        /**
         * @brief Get number of parameters
         * @return Number of parameters
         */
        int getNumParameters() const;
        
        /**
         * @brief Save plugin state
         * @return Serialized state
         */
        juce::MemoryBlock saveState() const;
        
        /**
         * @brief Load plugin state
         * @param state Serialized state
         * @return True if successful
         */
        bool loadState(const juce::MemoryBlock& state);
        
        /**
         * @brief Get plugin state as XML
         * @return XML state
         */
        juce::XmlElement* getStateAsXml() const;
        
        /**
         * @brief Set plugin state from XML
         * @param xml XML state
         * @return True if successful
         */
        bool setStateFromXml(const juce::XmlElement& xml);
        
    private:
        std::unique_ptr<juce::AudioPluginInstance> processor;
        PluginInfo info;
        mutable std::mutex parameterMutex;
    };
    
    /**
     * @class PluginHost
     * @brief High-performance plugin host with sandboxing
     * 
     * Provides safe plugin loading/unloading, parameter management,
     * state serialization, and dynamic scanning with metadata caching.
     */
    class PluginHost
    {
    public:
        /**
         * @brief Constructor
         */
        PluginHost();
        
        /**
         * @brief Destructor
         */
        ~PluginHost();
        
        /**
         * @brief Initialize the plugin host
         * @return true if successful
         */
        bool initialize();
        
        /**
         * @brief Shutdown the plugin host
         */
        void shutdown();
        
        /**
         * @brief Scan for plugins
         * @param directories Directories to scan
         * @return Number of plugins found
         */
        int scanForPlugins(const std::vector<juce::String>& directories = {});
        
        /**
         * @brief Get available plugins
         * @return Vector of plugin information
         */
        std::vector<PluginInfo> getAvailablePlugins() const;
        
        /**
         * @brief Load plugin
         * @param pluginInfo Plugin information
         * @param sampleRate Sample rate
         * @param bufferSize Buffer size
         * @return Plugin instance ID if successful, -1 if failed
         */
        int loadPlugin(const PluginInfo& pluginInfo, double sampleRate = 44100.0, int bufferSize = 512);
        
        /**
         * @brief Unload plugin
         * @param instanceId Plugin instance ID
         * @return True if successful
         */
        bool unloadPlugin(int instanceId);
        
        /**
         * @brief Get plugin instance
         * @param instanceId Plugin instance ID
         * @return Pointer to plugin instance, nullptr if not found
         */
        PluginInstance* getPluginInstance(int instanceId);
        
        /**
         * @brief Get all loaded plugin instances
         * @return Vector of plugin instance pointers
         */
        std::vector<PluginInstance*> getLoadedPlugins();
        
        /**
         * @brief Process audio with plugin
         * @param instanceId Plugin instance ID
         * @param buffer Audio buffer
         * @return True if successful
         */
        bool processPlugin(int instanceId, juce::AudioBuffer<float>& buffer);
        
        /**
         * @brief Set plugin parameter
         * @param instanceId Plugin instance ID
         * @param parameterIndex Parameter index
         * @param value Parameter value
         * @return True if successful
         */
        bool setPluginParameter(int instanceId, int parameterIndex, float value);
        
        /**
         * @brief Get plugin parameter
         * @param instanceId Plugin instance ID
         * @param parameterIndex Parameter index
         * @return Parameter value
         */
        float getPluginParameter(int instanceId, int parameterIndex) const;
        
        /**
         * @brief Save plugin state
         * @param instanceId Plugin instance ID
         * @return Serialized state
         */
        juce::MemoryBlock savePluginState(int instanceId) const;
        
        /**
         * @brief Load plugin state
         * @param instanceId Plugin instance ID
         * @param state Serialized state
         * @return True if successful
         */
        bool loadPluginState(int instanceId, const juce::MemoryBlock& state);
        
        /**
         * @brief Get plugin metadata
         * @param instanceId Plugin instance ID
         * @return Plugin information
         */
        PluginInfo getPluginMetadata(int instanceId) const;
        
        /**
         * @brief Add plugin callback
         * @param callback Function to call on plugin events
         */
        void addPluginCallback(std::function<void(int, const juce::String&)> callback);
        
        /**
         * @brief Remove plugin callback
         * @param callback Function to remove
         */
        void removePluginCallback(std::function<void(int, const juce::String&)> callback);
        
        /**
         * @brief Get plugin statistics
         * @return Statistics structure
         */
        struct PluginStats
        {
            int totalPlugins = 0;
            int loadedPlugins = 0;
            int vst3Plugins = 0;
            int auPlugins = 0;
            double averageLoadTime = 0.0;
            int crashedPlugins = 0;
        };
        
        PluginStats getPluginStats() const;
        
    private:
        // Plugin management
        std::unique_ptr<juce::AudioPluginFormatManager> formatManager;
        std::vector<PluginInfo> availablePlugins;
        std::unordered_map<int, std::unique_ptr<PluginInstance>> loadedPlugins;
        std::atomic<int> nextInstanceId{1};
        
        // Threading and safety
        mutable std::mutex pluginMutex;
        std::vector<std::function<void(int, const juce::String&)>> pluginCallbacks;
        std::mutex callbackMutex;
        
        // Statistics
        mutable std::atomic<int> totalPlugins{0};
        mutable std::atomic<int> loadedPluginsCount{0};
        mutable std::atomic<int> vst3Plugins{0};
        mutable std::atomic<int> auPlugins{0};
        mutable std::atomic<double> averageLoadTime{0.0};
        mutable std::atomic<int> crashedPlugins{0};
        
        // Internal methods
        void scanDirectory(const juce::String& directory);
        void addPlugin(const juce::PluginDescription& description, const juce::String& filePath);
        void notifyCallbacks(int instanceId, const juce::String& message);
        bool isPluginSafe(const juce::String& filePath) const;
    };
}