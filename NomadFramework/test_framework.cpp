/**
 * @file test_framework.cpp
 * @brief Simple test runner for Nomad Framework
 */

#include <iostream>
#include <chrono>
#include <thread>

// Include the framework
#include "nomad.h"

int main()
{
    std::cout << "=== Nomad Framework Test Runner ===" << std::endl;
    std::cout << "Testing framework initialization and basic functionality..." << std::endl;
    
    try
    {
        // Test 1: Framework Initialization
        std::cout << "\n1. Testing framework initialization..." << std::endl;
        if (!nomad::initialize(44100.0, 512))
        {
            std::cerr << "❌ FAILED: Framework initialization failed!" << std::endl;
            return -1;
        }
        std::cout << "✅ PASSED: Framework initialized successfully" << std::endl;
        std::cout << "   Version: " << nomad::getVersion() << std::endl;
        
        // Test 2: Audio Engine
        std::cout << "\n2. Testing audio engine..." << std::endl;
        auto& audioEngine = nomad::audio::AudioEngine::getInstance();
        if (!audioEngine.isInitialized())
        {
            std::cerr << "❌ FAILED: Audio engine not initialized!" << std::endl;
            return -1;
        }
        std::cout << "✅ PASSED: Audio engine initialized" << std::endl;
        std::cout << "   Sample rate: " << audioEngine.getSampleRate() << " Hz" << std::endl;
        std::cout << "   Buffer size: " << audioEngine.getBufferSize() << " samples" << std::endl;
        
        // Test 3: MIDI Engine
        std::cout << "\n3. Testing MIDI engine..." << std::endl;
        auto& midiEngine = nomad::midi::MidiEngine::getInstance();
        if (!midiEngine.isInitialized())
        {
            std::cerr << "❌ FAILED: MIDI engine not initialized!" << std::endl;
            return -1;
        }
        std::cout << "✅ PASSED: MIDI engine initialized" << std::endl;
        std::cout << "   Tempo: " << midiEngine.getTempo() << " BPM" << std::endl;
        
        // Test 4: Transport System
        std::cout << "\n4. Testing transport system..." << std::endl;
        auto& transport = nomad::transport::Transport::getInstance();
        if (!transport.isInitialized())
        {
            std::cerr << "❌ FAILED: Transport system not initialized!" << std::endl;
            return -1;
        }
        std::cout << "✅ PASSED: Transport system initialized" << std::endl;
        std::cout << "   Initial state: " << (transport.isPlaying() ? "Playing" : "Stopped") << std::endl;
        
        // Test 5: Parameter Manager
        std::cout << "\n5. Testing parameter manager..." << std::endl;
        auto& paramManager = nomad::parameters::ParameterManager::getInstance();
        if (!paramManager.isInitialized())
        {
            std::cerr << "❌ FAILED: Parameter manager not initialized!" << std::endl;
            return -1;
        }
        std::cout << "✅ PASSED: Parameter manager initialized" << std::endl;
        std::cout << "   Initial parameter count: " << paramManager.getNumParameters() << std::endl;
        
        // Test 6: Create and manipulate parameters
        std::cout << "\n6. Testing parameter creation and manipulation..." << std::endl;
        nomad::parameters::ParameterRange range(0.0, 100.0, 50.0, 0.1);
        auto* volumeParam = paramManager.createParameter("volume", "Volume", 
                                                        nomad::parameters::ParameterType::Float, range);
        if (!volumeParam)
        {
            std::cerr << "❌ FAILED: Could not create parameter!" << std::endl;
            return -1;
        }
        std::cout << "✅ PASSED: Parameter created successfully" << std::endl;
        
        // Test parameter value setting
        volumeParam->setScaledValue(0.75);
        double value = volumeParam->getScaledValue();
        if (std::abs(value - 0.75) > 0.001)
        {
            std::cerr << "❌ FAILED: Parameter value not set correctly!" << std::endl;
            return -1;
        }
        std::cout << "✅ PASSED: Parameter value manipulation works" << std::endl;
        std::cout << "   Set value: 0.75, Got value: " << value << std::endl;
        
        // Test 7: Transport control
        std::cout << "\n7. Testing transport control..." << std::endl;
        transport.play();
        if (!transport.isPlaying())
        {
            std::cerr << "❌ FAILED: Transport play failed!" << std::endl;
            return -1;
        }
        std::cout << "✅ PASSED: Transport play works" << std::endl;
        
        transport.pause();
        if (transport.isPlaying())
        {
            std::cerr << "❌ FAILED: Transport pause failed!" << std::endl;
            return -1;
        }
        std::cout << "✅ PASSED: Transport pause works" << std::endl;
        
        transport.stop();
        if (transport.isPlaying())
        {
            std::cerr << "❌ FAILED: Transport stop failed!" << std::endl;
            return -1;
        }
        std::cout << "✅ PASSED: Transport stop works" << std::endl;
        
        // Test 8: Real-time processing simulation
        std::cout << "\n8. Testing real-time processing simulation..." << std::endl;
        transport.play();
        
        const int numIterations = 10;
        const int samplesPerIteration = 512;
        
        for (int i = 0; i < numIterations; ++i)
        {
            transport.processTransport(samplesPerIteration);
            paramManager.updateParameters(samplesPerIteration);
            
            // Small delay to simulate real-time
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
        
        double finalTime = transport.getTimePosition();
        if (finalTime <= 0.0)
        {
            std::cerr << "❌ FAILED: Time did not advance during processing!" << std::endl;
            return -1;
        }
        std::cout << "✅ PASSED: Real-time processing simulation works" << std::endl;
        std::cout << "   Final time position: " << finalTime << " seconds" << std::endl;
        
        // Test 9: Performance stats
        std::cout << "\n9. Testing performance statistics..." << std::endl;
        auto audioStats = audioEngine.getPerformanceStats();
        auto midiStats = midiEngine.getMidiStats();
        auto paramStats = paramManager.getParameterStats();
        
        std::cout << "✅ PASSED: Performance statistics available" << std::endl;
        std::cout << "   Audio CPU usage: " << audioStats.cpuUsage << "%" << std::endl;
        std::cout << "   MIDI events processed: " << midiStats.eventsProcessed << std::endl;
        std::cout << "   Parameters created: " << paramStats.totalParameters << std::endl;
        
        // Test 10: Framework shutdown
        std::cout << "\n10. Testing framework shutdown..." << std::endl;
        nomad::shutdown();
        if (nomad::isInitialized())
        {
            std::cerr << "❌ FAILED: Framework shutdown failed!" << std::endl;
            return -1;
        }
        std::cout << "✅ PASSED: Framework shutdown successful" << std::endl;
        
        std::cout << "\n=== All Tests Passed! ===" << std::endl;
        std::cout << "The Nomad Framework is working correctly." << std::endl;
        
        return 0;
    }
    catch (const std::exception& e)
    {
        std::cerr << "❌ FAILED: Exception caught: " << e.what() << std::endl;
        return -1;
    }
    catch (...)
    {
        std::cerr << "❌ FAILED: Unknown exception caught!" << std::endl;
        return -1;
    }
}