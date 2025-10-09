/**
 * @file PluginHost.cpp
 * @brief VST3/AU plugin host implementation
 */

#include "plugins/PluginHost.h"
#include <algorithm>
#include <chrono>

namespace nomad::plugins
{
    // PluginInstance implementation
    PluginInstance::PluginInstance(std::unique_ptr<juce::AudioPluginInstance> processor, const PluginInfo& info)
        : processor(std::move(processor)), info(info)
    {
    }
    
    PluginInstance::~PluginInstance()
    {
        unload();
    }
    
    bool PluginInstance::load(double sampleRate, int bufferSize)
    {
        try
        {
            if (processor)
            {
                processor->prepareToPlay(sampleRate, bufferSize);
                info.isLoaded = true;
                return true;
            }
        }
        catch (const std::exception& e)
        {
            // Log error
        }
        return false;
    }
    
    void PluginInstance::unload()
    {
        if (processor && info.isLoaded)
        {
            processor->releaseResources();
            info.isLoaded = false;
        }
    }
    
    float PluginInstance::getParameterValue(int parameterIndex) const
    {
        std::lock_guard<std::mutex> lock(parameterMutex);
        if (processor && parameterIndex < processor->getNumParameters())
        {
            return processor->getParameter(parameterIndex);
        }
        return 0.0f;
    }
    
    void PluginInstance::setParameterValue(int parameterIndex, float value)
    {
        std::lock_guard<std::mutex> lock(parameterMutex);
        if (processor && parameterIndex < processor->getNumParameters())
        {
            processor->setParameter(parameterIndex, value);
        }
    }
    
    juce::String PluginInstance::getParameterName(int parameterIndex) const
    {
        if (processor && parameterIndex < processor->getNumParameters())
        {
            return processor->getParameterName(parameterIndex);
        }
        return juce::String();
    }
    
    juce::String PluginInstance::getParameterText(int parameterIndex, float value) const
    {
        if (processor && parameterIndex < processor->getNumParameters())
        {
            return processor->getParameterText(parameterIndex, value);
        }
        return juce::String();
    }
    
    int PluginInstance::getNumParameters() const
    {
        if (processor)
        {
            return processor->getNumParameters();
        }
        return 0;
    }
    
    juce::MemoryBlock PluginInstance::saveState() const
    {
        juce::MemoryBlock state;
        if (processor)
        {
            processor->getStateInformation(state);
        }
        return state;
    }
    
    bool PluginInstance::loadState(const juce::MemoryBlock& state)
    {
        if (processor)
        {
            processor->setStateInformation(state.getData(), static_cast<int>(state.getSize()));
            return true;
        }
        return false;
    }
    
    juce::XmlElement* PluginInstance::getStateAsXml() const
    {
        if (processor)
        {
            return processor->getXmlFromState();
        }
        return nullptr;
    }
    
    bool PluginInstance::setStateFromXml(const juce::XmlElement& xml)
    {
        if (processor)
        {
            processor->setStateFromXml(xml);
            return true;
        }
        return false;
    }
    
    // PluginHost implementation
    PluginHost::PluginHost()
    {
        formatManager = std::make_unique<juce::AudioPluginFormatManager>();
        formatManager->addDefaultFormats();
    }
    
    PluginHost::~PluginHost()
    {
        shutdown();
    }
    
    bool PluginHost::initialize()
    {
        try
        {
            // Initialize plugin formats
            formatManager->addDefaultFormats();
            
            // Scan default plugin directories
            std::vector<juce::String> defaultDirs;
            
            // Add system plugin directories
            #if JUCE_MAC
            defaultDirs.push_back("/Library/Audio/Plug-Ins/VST3");
            defaultDirs.push_back("/Library/Audio/Plug-Ins/Components");
            defaultDirs.push_back("~/Library/Audio/Plug-Ins/VST3");
            defaultDirs.push_back("~/Library/Audio/Plug-Ins/Components");
            #elif JUCE_WINDOWS
            defaultDirs.push_back("C:\\Program Files\\Common Files\\VST3");
            defaultDirs.push_back("C:\\Program Files\\VSTPlugins");
            defaultDirs.push_back("C:\\Program Files (x86)\\Common Files\\VST3");
            defaultDirs.push_back("C:\\Program Files (x86)\\VSTPlugins");
            #elif JUCE_LINUX
            defaultDirs.push_back("/usr/lib/vst3");
            defaultDirs.push_back("/usr/local/lib/vst3");
            defaultDirs.push_back("~/.vst3");
            #endif
            
            scanForPlugins(defaultDirs);
            
            return true;
        }
        catch (const std::exception& e)
        {
            return false;
        }
    }
    
    void PluginHost::shutdown()
    {
        std::lock_guard<std::mutex> lock(pluginMutex);
        
        // Unload all plugins
        for (auto& pair : loadedPlugins)
        {
            pair.second->unload();
        }
        loadedPlugins.clear();
        
        availablePlugins.clear();
    }
    
    int PluginHost::scanForPlugins(const std::vector<juce::String>& directories)
    {
        int pluginsFound = 0;
        
        for (const auto& directory : directories)
        {
            scanDirectory(directory);
        }
        
        totalPlugins = static_cast<int>(availablePlugins.size());
        return static_cast<int>(availablePlugins.size());
    }
    
    std::vector<PluginInfo> PluginHost::getAvailablePlugins() const
    {
        std::lock_guard<std::mutex> lock(pluginMutex);
        return availablePlugins;
    }
    
    int PluginHost::loadPlugin(const PluginInfo& pluginInfo, double sampleRate, int bufferSize)
    {
        std::lock_guard<std::mutex> lock(pluginMutex);
        
        try
        {
            auto startTime = std::chrono::high_resolution_clock::now();
            
            // Create plugin instance
            juce::String error;
            auto processor = formatManager->createPluginInstance(pluginInfo.description, sampleRate, bufferSize, error);
            
            if (!processor)
            {
                notifyCallbacks(-1, "Failed to create plugin instance: " + error);
                return -1;
            }
            
            // Create wrapper
            auto instance = std::make_unique<PluginInstance>(std::move(processor), pluginInfo);
            
            // Load plugin
            if (!instance->load(sampleRate, bufferSize))
            {
                notifyCallbacks(-1, "Failed to load plugin");
                return -1;
            }
            
            // Add to loaded plugins
            int instanceId = nextInstanceId++;
            loadedPlugins[instanceId] = std::move(instance);
            loadedPluginsCount++;
            
            // Update statistics
            auto endTime = std::chrono::high_resolution_clock::now();
            auto loadTime = std::chrono::duration<double, std::milli>(endTime - startTime).count();
            
            double currentAvg = averageLoadTime.load();
            int loadedCount = loadedPluginsCount.load();
            averageLoadTime = (currentAvg * (loadedCount - 1) + loadTime) / loadedCount;
            
            // Update plugin type counts
            if (pluginInfo.isVST3)
                vst3Plugins++;
            if (pluginInfo.isAU)
                auPlugins++;
            
            notifyCallbacks(instanceId, "Plugin loaded successfully");
            return instanceId;
        }
        catch (const std::exception& e)
        {
            crashedPlugins++;
            notifyCallbacks(-1, "Plugin crashed during loading: " + juce::String(e.what()));
            return -1;
        }
    }
    
    bool PluginHost::unloadPlugin(int instanceId)
    {
        std::lock_guard<std::mutex> lock(pluginMutex);
        
        auto it = loadedPlugins.find(instanceId);
        if (it != loadedPlugins.end())
        {
            it->second->unload();
            loadedPlugins.erase(it);
            loadedPluginsCount--;
            
            notifyCallbacks(instanceId, "Plugin unloaded");
            return true;
        }
        
        return false;
    }
    
    PluginInstance* PluginHost::getPluginInstance(int instanceId)
    {
        std::lock_guard<std::mutex> lock(pluginMutex);
        
        auto it = loadedPlugins.find(instanceId);
        if (it != loadedPlugins.end())
        {
            return it->second.get();
        }
        
        return nullptr;
    }
    
    std::vector<PluginInstance*> PluginHost::getLoadedPlugins()
    {
        std::lock_guard<std::mutex> lock(pluginMutex);
        
        std::vector<PluginInstance*> plugins;
        for (auto& pair : loadedPlugins)
        {
            plugins.push_back(pair.second.get());
        }
        
        return plugins;
    }
    
    bool PluginHost::processPlugin(int instanceId, juce::AudioBuffer<float>& buffer)
    {
        auto* instance = getPluginInstance(instanceId);
        if (!instance || !instance->isLoaded())
            return false;
        
        try
        {
            auto& processor = instance->getProcessor();
            processor.processBlock(buffer, juce::MidiBuffer());
            return true;
        }
        catch (const std::exception& e)
        {
            crashedPlugins++;
            notifyCallbacks(instanceId, "Plugin crashed during processing: " + juce::String(e.what()));
            return false;
        }
    }
    
    bool PluginHost::setPluginParameter(int instanceId, int parameterIndex, float value)
    {
        auto* instance = getPluginInstance(instanceId);
        if (!instance)
            return false;
        
        instance->setParameterValue(parameterIndex, value);
        return true;
    }
    
    float PluginHost::getPluginParameter(int instanceId, int parameterIndex) const
    {
        auto* instance = const_cast<PluginHost*>(this)->getPluginInstance(instanceId);
        if (!instance)
            return 0.0f;
        
        return instance->getParameterValue(parameterIndex);
    }
    
    juce::MemoryBlock PluginHost::savePluginState(int instanceId) const
    {
        auto* instance = const_cast<PluginHost*>(this)->getPluginInstance(instanceId);
        if (!instance)
            return juce::MemoryBlock();
        
        return instance->saveState();
    }
    
    bool PluginHost::loadPluginState(int instanceId, const juce::MemoryBlock& state)
    {
        auto* instance = getPluginInstance(instanceId);
        if (!instance)
            return false;
        
        return instance->loadState(state);
    }
    
    PluginInfo PluginHost::getPluginMetadata(int instanceId) const
    {
        auto* instance = const_cast<PluginHost*>(this)->getPluginInstance(instanceId);
        if (!instance)
            return PluginInfo();
        
        return instance->getInfo();
    }
    
    void PluginHost::addPluginCallback(std::function<void(int, const juce::String&)> callback)
    {
        std::lock_guard<std::mutex> lock(callbackMutex);
        pluginCallbacks.push_back(callback);
    }
    
    void PluginHost::removePluginCallback(std::function<void(int, const juce::String&)> callback)
    {
        std::lock_guard<std::mutex> lock(callbackMutex);
        pluginCallbacks.erase(
            std::remove(pluginCallbacks.begin(), pluginCallbacks.end(), callback),
            pluginCallbacks.end()
        );
    }
    
    PluginHost::PluginStats PluginHost::getPluginStats() const
    {
        PluginStats stats;
        stats.totalPlugins = totalPlugins.load();
        stats.loadedPlugins = loadedPluginsCount.load();
        stats.vst3Plugins = vst3Plugins.load();
        stats.auPlugins = auPlugins.load();
        stats.averageLoadTime = averageLoadTime.load();
        stats.crashedPlugins = crashedPlugins.load();
        return stats;
    }
    
    void PluginHost::scanDirectory(const juce::String& directory)
    {
        juce::File dir(directory);
        if (!dir.exists() || !dir.isDirectory())
            return;
        
        for (auto& format : formatManager->getFormats())
        {
            juce::PluginDirectoryScanner scanner(*format, *formatManager, dir, true, juce::File());
            
            juce::String pluginName;
            while (scanner.scanNextFile(false, pluginName))
            {
                auto description = scanner.getNextPlugin();
                if (description)
                {
                    addPlugin(*description, scanner.getNextPluginFile().getFullPathName());
                }
            }
        }
    }
    
    void PluginHost::addPlugin(const juce::PluginDescription& description, const juce::String& filePath)
    {
        PluginInfo info;
        info.name = description.name;
        info.manufacturer = description.manufacturerName;
        info.version = description.version;
        info.category = description.category;
        info.filePath = filePath;
        info.description = description;
        info.isVST3 = description.pluginFormatName == "VST3";
        info.isAU = description.pluginFormatName == "AudioUnit";
        info.isLoaded = false;
        
        availablePlugins.push_back(info);
    }
    
    void PluginHost::notifyCallbacks(int instanceId, const juce::String& message)
    {
        std::lock_guard<std::mutex> lock(callbackMutex);
        for (auto& callback : pluginCallbacks)
        {
            callback(instanceId, message);
        }
    }
    
    bool PluginHost::isPluginSafe(const juce::String& filePath) const
    {
        // Basic safety checks
        juce::File pluginFile(filePath);
        if (!pluginFile.exists())
            return false;
        
        // Check file size (reasonable limits)
        if (pluginFile.getSize() > 100 * 1024 * 1024) // 100MB limit
            return false;
        
        // Check file extension
        juce::String extension = pluginFile.getFileExtension().toLowerCase();
        if (extension != ".vst3" && extension != ".component" && extension != ".so")
            return false;
        
        return true;
    }
}