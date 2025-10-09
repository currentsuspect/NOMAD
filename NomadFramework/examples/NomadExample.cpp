/**
 * @file NomadExample.cpp
 * @brief Example application demonstrating the Nomad Framework
 */

#include <JuceHeader.h>
#include "nomad.h"
#include <iostream>
#include <thread>
#include <chrono>

class NomadExampleApplication : public juce::JUCEApplication
{
public:
    NomadExampleApplication() = default;
    
    const juce::String getApplicationName() override { return "Nomad Framework Example"; }
    const juce::String getApplicationVersion() override { return "1.0.0"; }
    bool moreThanOneInstanceAllowed() override { return true; }
    
    void initialise(const juce::String& commandLine) override
    {
        std::cout << "Initializing Nomad Framework Example..." << std::endl;
        
        // Initialize the Nomad Framework
        if (!nomad::initialize(44100.0, 512))
        {
            std::cerr << "Failed to initialize Nomad Framework!" << std::endl;
            return;
        }
        
        std::cout << "Nomad Framework initialized successfully!" << std::endl;
        std::cout << "Version: " << nomad::getVersion() << std::endl;
        
        // Demonstrate framework components
        demonstrateAudioEngine();
        demonstrateMidiEngine();
        demonstrateTransport();
        demonstrateParameters();
        demonstrateAutomation();
        demonstrateProjectSystem();
        
        std::cout << "Nomad Framework Example completed successfully!" << std::endl;
    }
    
    void shutdown() override
    {
        std::cout << "Shutting down Nomad Framework..." << std::endl;
        nomad::shutdown();
    }
    
    void systemRequestedQuit() override
    {
        quit();
    }
    
    void anotherInstanceStarted(const juce::String& commandLine) override
    {
        // Handle another instance
    }
    
private:
    void demonstrateAudioEngine()
    {
        std::cout << "\n=== Audio Engine Demo ===" << std::endl;
        
        // Get audio engine reference
        auto& audioEngine = nomad::audio::AudioEngine::getInstance();
        
        // Demonstrate buffer size changes
        std::cout << "Original buffer size: " << audioEngine.getBufferSize() << std::endl;
        audioEngine.setBufferSize(1024);
        std::cout << "New buffer size: " << audioEngine.getBufferSize() << std::endl;
        
        // Demonstrate sample rate changes
        std::cout << "Original sample rate: " << audioEngine.getSampleRate() << std::endl;
        audioEngine.setSampleRate(48000.0);
        std::cout << "New sample rate: " << audioEngine.getSampleRate() << std::endl;
        
        // Get performance stats
        auto stats = audioEngine.getPerformanceStats();
        std::cout << "CPU Usage: " << stats.cpuUsage << "%" << std::endl;
        std::cout << "Max CPU Usage: " << stats.maxCpuUsage << "%" << std::endl;
        std::cout << "Buffer Underruns: " << stats.bufferUnderruns << std::endl;
        std::cout << "Average Latency: " << stats.averageLatency << " ms" << std::endl;
    }
    
    void demonstrateMidiEngine()
    {
        std::cout << "\n=== MIDI Engine Demo ===" << std::endl;
        
        // Get MIDI engine reference
        auto& midiEngine = nomad::midi::MidiEngine::getInstance();
        
        // Demonstrate tempo changes
        std::cout << "Original tempo: " << midiEngine.getTempo() << " BPM" << std::endl;
        midiEngine.setTempo(140.0);
        std::cout << "New tempo: " << midiEngine.getTempo() << " BPM" << std::endl;
        
        // Demonstrate quantization
        midiEngine.setQuantizationEnabled(true);
        midiEngine.setQuantizationGrid(0.25);
        std::cout << "Quantization enabled: " << (midiEngine.isQuantizationEnabled() ? "Yes" : "No") << std::endl;
        std::cout << "Quantization grid: " << midiEngine.getQuantizationGrid() << " beats" << std::endl;
        
        // Demonstrate clock sync
        midiEngine.setClockSyncEnabled(true);
        std::cout << "Clock sync enabled: " << (midiEngine.isClockSyncEnabled() ? "Yes" : "No") << std::endl;
        
        // Get MIDI stats
        auto stats = midiEngine.getMidiStats();
        std::cout << "Events processed: " << stats.eventsProcessed << std::endl;
        std::cout << "Events dropped: " << stats.eventsDropped << std::endl;
        std::cout << "Average latency: " << stats.averageLatency << " ms" << std::endl;
        std::cout << "Active inputs: " << stats.activeInputs << std::endl;
        std::cout << "Active outputs: " << stats.activeOutputs << std::endl;
    }
    
    void demonstrateTransport()
    {
        std::cout << "\n=== Transport Demo ===" << std::endl;
        
        // Get transport reference
        auto& transport = nomad::transport::Transport::getInstance();
        
        // Demonstrate playback control
        std::cout << "Initial state: " << (transport.isPlaying() ? "Playing" : "Stopped") << std::endl;
        
        transport.play();
        std::cout << "After play: " << (transport.isPlaying() ? "Playing" : "Stopped") << std::endl;
        
        transport.pause();
        std::cout << "After pause: " << (transport.isPlaying() ? "Playing" : "Paused") << std::endl;
        
        transport.stop();
        std::cout << "After stop: " << (transport.isPlaying() ? "Playing" : "Stopped") << std::endl;
        
        // Demonstrate time positioning
        transport.setTimePosition(10.5);
        std::cout << "Time position: " << transport.getTimePosition() << " seconds" << std::endl;
        
        transport.setBeatPosition(4.0);
        std::cout << "Beat position: " << transport.getBeatPosition() << " beats" << std::endl;
        
        // Demonstrate tempo changes
        transport.setTempo(120.0);
        std::cout << "Tempo: " << transport.getTempo() << " BPM" << std::endl;
        
        // Demonstrate time signature
        transport.setTimeSignature(3, 4);
        std::cout << "Time signature: " << transport.getTimeSignatureNumerator() 
                  << "/" << transport.getTimeSignatureDenominator() << std::endl;
        
        // Demonstrate looping
        transport.setLoopEnabled(true);
        transport.setLoopRange(5.0, 15.0);
        std::cout << "Loop enabled: " << (transport.isLoopEnabled() ? "Yes" : "No") << std::endl;
        std::cout << "Loop range: " << transport.getLoopStart() << " - " << transport.getLoopEnd() << " seconds" << std::endl;
        
        // Demonstrate time conversions
        double time = transport.beatsToTime(4.0);
        double beats = transport.timeToBeats(time);
        std::cout << "4 beats = " << time << " seconds" << std::endl;
        std::cout << time << " seconds = " << beats << " beats" << std::endl;
    }
    
    void demonstrateParameters()
    {
        std::cout << "\n=== Parameter Manager Demo ===" << std::endl;
        
        // Get parameter manager reference
        auto& paramManager = nomad::parameters::ParameterManager::getInstance();
        
        // Create some parameters
        nomad::parameters::ParameterRange floatRange(0.0, 100.0, 50.0, 0.1);
        nomad::parameters::ParameterRange intRange(0, 127, 64, 1);
        nomad::parameters::ParameterRange boolRange(0, 1, 0, 1);
        
        auto* floatParam = paramManager.createParameter("volume", "Volume", 
                                                       nomad::parameters::ParameterType::Float, floatRange);
        auto* intParam = paramManager.createParameter("midi_channel", "MIDI Channel", 
                                                     nomad::parameters::ParameterType::Int, intRange);
        auto* boolParam = paramManager.createParameter("mute", "Mute", 
                                                      nomad::parameters::ParameterType::Bool, boolRange);
        
        std::cout << "Created " << paramManager.getNumParameters() << " parameters" << std::endl;
        
        // Demonstrate parameter operations
        if (floatParam)
        {
            floatParam->setScaledValue(0.75);
            std::cout << "Volume parameter: " << floatParam->getScaledValue() 
                      << " (raw: " << floatParam->getRawValue() << ")" << std::endl;
        }
        
        if (intParam)
        {
            intParam->setScaledValue(0.5);
            std::cout << "MIDI Channel parameter: " << intParam->getScaledValue() 
                      << " (raw: " << intParam->getRawValue() << ")" << std::endl;
        }
        
        if (boolParam)
        {
            boolParam->setScaledValue(1.0);
            std::cout << "Mute parameter: " << (boolParam->getScaledValue() > 0.5 ? "On" : "Off") << std::endl;
        }
        
        // Demonstrate parameter groups
        std::vector<std::string> audioParams = {"volume", "mute"};
        paramManager.addParameterGroup("audio", audioParams);
        std::cout << "Created parameter group 'audio' with " << paramManager.getParameterGroup("audio").size() << " parameters" << std::endl;
        
        // Demonstrate parameter statistics
        auto stats = paramManager.getParameterStats();
        std::cout << "Parameter statistics:" << std::endl;
        std::cout << "  Total parameters: " << stats.totalParameters << std::endl;
        std::cout << "  Float parameters: " << stats.floatParameters << std::endl;
        std::cout << "  Int parameters: " << stats.intParameters << std::endl;
        std::cout << "  Bool parameters: " << stats.boolParameters << std::endl;
        std::cout << "  Parameter groups: " << stats.parameterGroups << std::endl;
    }
    
    void demonstrateAutomation()
    {
        std::cout << "\n=== Automation Engine Demo ===" << std::endl;
        
        // Get automation engine reference
        auto& automationEngine = nomad::automation::AutomationEngine::getInstance();
        
        // Create automation lanes
        int volumeLane = automationEngine.createAutomationLane("volume");
        int panLane = automationEngine.createAutomationLane("pan");
        
        std::cout << "Created automation lanes: " << volumeLane << ", " << panLane << std::endl;
        
        // Set up keyframe automation for volume
        automationEngine.setAutomationLaneType(volumeLane, nomad::automation::AutomationType::Keyframe);
        automationEngine.addKeyframe(volumeLane, nomad::automation::AutomationPoint(0.0, 0.0, 0.0));
        automationEngine.addKeyframe(volumeLane, nomad::automation::AutomationPoint(5.0, 1.0, 0.0));
        automationEngine.addKeyframe(volumeLane, nomad::automation::AutomationPoint(10.0, 0.5, 0.0));
        
        std::cout << "Added keyframes to volume lane" << std::endl;
        
        // Set up LFO automation for pan
        automationEngine.setAutomationLaneType(panLane, nomad::automation::AutomationType::LFO);
        nomad::automation::LFOData lfoData;
        lfoData.type = nomad::automation::LFOType::Sine;
        lfoData.frequency = 0.5; // 0.5 Hz
        lfoData.amplitude = 0.5;
        lfoData.offset = 0.5;
        automationEngine.setLFOData(panLane, lfoData);
        
        std::cout << "Set up LFO automation for pan lane" << std::endl;
        
        // Process automation
        automationEngine.setCurrentTime(0.0);
        automationEngine.processAutomation(512, 120.0);
        
        std::cout << "Processed automation for 512 samples" << std::endl;
        
        // Get automation statistics
        auto stats = automationEngine.getAutomationStats();
        std::cout << "Automation statistics:" << std::endl;
        std::cout << "  Total lanes: " << stats.totalLanes << std::endl;
        std::cout << "  Active lanes: " << stats.activeLanes << std::endl;
        std::cout << "  Keyframe lanes: " << stats.keyframeLanes << std::endl;
        std::cout << "  LFO lanes: " << stats.lfoLanes << std::endl;
        std::cout << "  Total keyframes: " << stats.totalKeyframes << std::endl;
    }
    
    void demonstrateProjectSystem()
    {
        std::cout << "\n=== Project Manager Demo ===" << std::endl;
        
        // Get project manager reference
        auto& projectManager = nomad::project::ProjectManager::getInstance();
        
        // Create a new project
        projectManager.createNewProject("Test Project", 44100.0, 512);
        std::cout << "Created new project: " << projectManager.getCurrentProjectInfo().name << std::endl;
        
        // Add some resources
        nomad::project::ResourceInfo sample1;
        sample1.id = "sample_1";
        sample1.name = "Kick Drum";
        sample1.type = "audio_sample";
        sample1.filePath = "/path/to/kick.wav";
        sample1.size = 1024000;
        sample1.isLoaded = false;
        projectManager.addResource(sample1);
        
        nomad::project::ResourceInfo sample2;
        sample2.id = "sample_2";
        sample2.name = "Snare Drum";
        sample2.type = "audio_sample";
        sample2.filePath = "/path/to/snare.wav";
        sample2.size = 512000;
        sample2.isLoaded = true;
        projectManager.addResource(sample2);
        
        std::cout << "Added " << projectManager.getNumParameters() << " resources to project" << std::endl;
        
        // Demonstrate project statistics
        auto stats = projectManager.getProjectStats();
        std::cout << "Project statistics:" << std::endl;
        std::cout << "  Total resources: " << stats.totalResources << std::endl;
        std::cout << "  Loaded resources: " << stats.loadedResources << std::endl;
        std::cout << "  Total resource size: " << stats.totalResourceSize << " bytes" << std::endl;
        
        // Demonstrate autosave
        projectManager.setAutosaveEnabled(true);
        projectManager.setAutosaveInterval(300000); // 5 minutes
        std::cout << "Autosave enabled: " << (projectManager.isAutosaveEnabled() ? "Yes" : "No") << std::endl;
        std::cout << "Autosave interval: " << projectManager.getAutosaveInterval() << " ms" << std::endl;
        
        // Demonstrate crash recovery
        projectManager.setCrashRecoveryEnabled(true);
        std::cout << "Crash recovery enabled: " << (projectManager.isCrashRecoveryEnabled() ? "Yes" : "No") << std::endl;
        
        // Create a backup
        projectManager.createBackup();
        std::cout << "Created project backup" << std::endl;
        
        // Get available backups
        auto backups = projectManager.getAvailableBackups();
        std::cout << "Available backups: " << backups.size() << std::endl;
    }
};

// This macro creates the application's main() function
START_JUCE_APPLICATION(NomadExampleApplication)