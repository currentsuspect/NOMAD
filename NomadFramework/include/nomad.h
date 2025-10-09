/**
 * @file nomad.h
 * @brief Main header file for the Nomad Framework
 * @author Nomad Framework Team
 * @version 1.0.0
 * 
 * This file provides the main entry point for the Nomad Framework,
 * a high-performance, low-latency audio and system framework.
 */

#pragma once

// Core includes
#include "audio/AudioEngine.h"
#include "midi/MidiEngine.h"
#include "transport/Transport.h"
#include "plugins/PluginHost.h"
#include "project/ProjectManager.h"
#include "automation/AutomationEngine.h"
#include "parameters/ParameterManager.h"
#include "state/StateManager.h"
#include "utils/PerformanceProfiler.h"

/**
 * @namespace nomad
 * @brief Main namespace for the Nomad Framework
 * 
 * The nomad namespace contains all framework components organized
 * into sub-namespaces for different functional areas.
 */
namespace nomad
{
    /**
     * @brief Initialize the Nomad Framework
     * @param sampleRate The audio sample rate
     * @param bufferSize The audio buffer size
     * @return true if initialization was successful
     * 
     * This function must be called before using any framework components.
     * It initializes all subsystems and prepares them for operation.
     */
    bool initialize(double sampleRate = 44100.0, int bufferSize = 512);
    
    /**
     * @brief Shutdown the Nomad Framework
     * 
     * Cleanly shuts down all framework components and releases resources.
     * Should be called when the application is closing.
     */
    void shutdown();
    
    /**
     * @brief Get the current framework version
     * @return Version string
     */
    const char* getVersion();
    
    /**
     * @brief Check if the framework is initialized
     * @return true if initialized
     */
    bool isInitialized();
}