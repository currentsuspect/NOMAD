/**
 * @file nomad.cpp
 * @brief Main implementation file for the Nomad Framework
 */

#include "nomad.h"
#include <memory>
#include <atomic>

namespace nomad
{
    namespace
    {
        std::atomic<bool> g_initialized{false};
        std::unique_ptr<AudioEngine> g_audioEngine;
        std::unique_ptr<MidiEngine> g_midiEngine;
        std::unique_ptr<Transport> g_transport;
        std::unique_ptr<PluginHost> g_pluginHost;
        std::unique_ptr<ProjectManager> g_projectManager;
        std::unique_ptr<AutomationEngine> g_automationEngine;
        std::unique_ptr<ParameterManager> g_parameterManager;
        std::unique_ptr<StateManager> g_stateManager;
    }
    
    bool initialize(double sampleRate, int bufferSize)
    {
        if (g_initialized.load())
            return true;
            
        try
        {
            // Initialize core components
            g_audioEngine = std::make_unique<AudioEngine>(sampleRate, bufferSize);
            g_midiEngine = std::make_unique<MidiEngine>();
            g_transport = std::make_unique<Transport>(*g_audioEngine);
            g_pluginHost = std::make_unique<PluginHost>();
            g_projectManager = std::make_unique<ProjectManager>();
            g_automationEngine = std::make_unique<AutomationEngine>(*g_audioEngine);
            g_parameterManager = std::make_unique<ParameterManager>();
            g_stateManager = std::make_unique<StateManager>();
            
            // Initialize subsystems
            g_audioEngine->initialize();
            g_midiEngine->initialize();
            g_transport->initialize();
            g_pluginHost->initialize();
            g_projectManager->initialize();
            g_automationEngine->initialize();
            g_parameterManager->initialize();
            g_stateManager->initialize();
            
            g_initialized.store(true);
            return true;
        }
        catch (const std::exception& e)
        {
            // Log error
            return false;
        }
    }
    
    void shutdown()
    {
        if (!g_initialized.load())
            return;
            
        // Shutdown in reverse order
        g_stateManager.reset();
        g_parameterManager.reset();
        g_automationEngine.reset();
        g_projectManager.reset();
        g_pluginHost.reset();
        g_transport.reset();
        g_midiEngine.reset();
        g_audioEngine.reset();
        
        g_initialized.store(false);
    }
    
    const char* getVersion()
    {
        return "1.0.0";
    }
    
    bool isInitialized()
    {
        return g_initialized.load();
    }
}