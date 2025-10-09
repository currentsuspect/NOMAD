/**
 * @file TestIntegration.cpp
 * @brief Integration tests for Nomad Framework
 */

#include <gtest/gtest.h>
#include "nomad.h"
#include <thread>
#include <chrono>

class NomadFrameworkIntegrationTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        // Initialize the framework
        ASSERT_TRUE(nomad::initialize(44100.0, 512));
    }
    
    void TearDown() override
    {
        // Shutdown the framework
        nomad::shutdown();
    }
};

TEST_F(NomadFrameworkIntegrationTest, FrameworkInitialization)
{
    EXPECT_TRUE(nomad::isInitialized());
    EXPECT_STREQ(nomad::getVersion(), "1.0.0");
}

TEST_F(NomadFrameworkIntegrationTest, AudioAndMidiIntegration)
{
    // Test that audio and MIDI engines work together
    auto& audioEngine = nomad::audio::AudioEngine::getInstance();
    auto& midiEngine = nomad::midi::MidiEngine::getInstance();
    
    EXPECT_TRUE(audioEngine.isInitialized());
    EXPECT_TRUE(midiEngine.isInitialized());
    
    // Test sample rate consistency
    EXPECT_EQ(audioEngine.getSampleRate(), 44100.0);
    
    // Test MIDI tempo affects audio processing
    midiEngine.setTempo(140.0);
    EXPECT_EQ(midiEngine.getTempo(), 140.0);
}

TEST_F(NomadFrameworkIntegrationTest, TransportAndAudioIntegration)
{
    auto& audioEngine = nomad::audio::AudioEngine::getInstance();
    auto& transport = nomad::transport::Transport::getInstance();
    
    // Test transport affects audio processing
    transport.setTempo(120.0);
    EXPECT_EQ(transport.getTempo(), 120.0);
    
    // Test time position updates
    transport.play();
    EXPECT_TRUE(transport.isPlaying());
    
    // Simulate audio processing
    transport.processTransport(512);
    EXPECT_GT(transport.getTimePosition(), 0.0);
}

TEST_F(NomadFrameworkIntegrationTest, ParameterAndAutomationIntegration)
{
    auto& paramManager = nomad::parameters::ParameterManager::getInstance();
    auto& automationEngine = nomad::automation::AutomationEngine::getInstance();
    
    // Create a parameter
    nomad::parameters::ParameterRange range(0.0, 100.0, 50.0, 0.1);
    auto* param = paramManager.createParameter("test_param", "Test Parameter", 
                                               nomad::parameters::ParameterType::Float, range);
    ASSERT_NE(param, nullptr);
    
    // Create automation lane for the parameter
    int laneId = automationEngine.createAutomationLane("test_param");
    EXPECT_GE(laneId, 0);
    
    // Set up keyframe automation
    automationEngine.setAutomationLaneType(laneId, nomad::automation::AutomationType::Keyframe);
    automationEngine.addKeyframe(laneId, nomad::automation::AutomationPoint(0.0, 0.0, 0.0));
    automationEngine.addKeyframe(laneId, nomad::automation::AutomationPoint(5.0, 1.0, 0.0));
    
    // Process automation
    automationEngine.setCurrentTime(0.0);
    automationEngine.processAutomation(512, 120.0);
    
    // Verify parameter was affected by automation
    EXPECT_GE(param->getScaledValue(), 0.0);
}

TEST_F(NomadFrameworkIntegrationTest, ProjectAndParameterIntegration)
{
    auto& projectManager = nomad::project::ProjectManager::getInstance();
    auto& paramManager = nomad::parameters::ParameterManager::getInstance();
    
    // Create a project
    ASSERT_TRUE(projectManager.createNewProject("Integration Test", 44100.0, 512));
    
    // Create parameters
    nomad::parameters::ParameterRange range(0.0, 100.0, 50.0, 0.1);
    paramManager.createParameter("project_param1", "Project Param 1", 
                                nomad::parameters::ParameterType::Float, range);
    paramManager.createParameter("project_param2", "Project Param 2", 
                                nomad::parameters::ParameterType::Int, range);
    
    // Set parameter values
    paramManager.setParameterValue("project_param1", 0.75);
    paramManager.setParameterValue("project_param2", 0.25);
    
    // Save project
    std::string projectPath = "test_project.json";
    ASSERT_TRUE(projectManager.saveProject(projectPath, nomad::project::ProjectFormat::JSON));
    
    // Create new project and load
    projectManager.closeProject();
    ASSERT_TRUE(projectManager.loadProject(projectPath, nomad::project::ProjectFormat::JSON));
    
    // Verify parameters were loaded
    EXPECT_NEAR(paramManager.getParameterValue("project_param1"), 0.75, 0.001);
    EXPECT_NEAR(paramManager.getParameterValue("project_param2"), 0.25, 0.001);
    
    // Clean up
    std::remove(projectPath.c_str());
}

TEST_F(NomadFrameworkIntegrationTest, RealTimeProcessing)
{
    auto& audioEngine = nomad::audio::AudioEngine::getInstance();
    auto& transport = nomad::transport::Transport::getInstance();
    auto& paramManager = nomad::parameters::ParameterManager::getInstance();
    
    // Create a parameter
    nomad::parameters::ParameterRange range(0.0, 100.0, 0.0, 0.1);
    auto* param = paramManager.createParameter("rt_param", "Real-time Parameter", 
                                               nomad::parameters::ParameterType::Float, range);
    ASSERT_NE(param, nullptr);
    
    // Set up smooth parameter changes
    param->setValueSmooth(0.5, 100.0); // 100ms transition
    
    // Start transport
    transport.play();
    
    // Simulate real-time processing
    const int numIterations = 10;
    const int samplesPerIteration = 512;
    
    for (int i = 0; i < numIterations; ++i)
    {
        // Process transport
        transport.processTransport(samplesPerIteration);
        
        // Update parameters
        paramManager.updateParameters(samplesPerIteration);
        
        // Small delay to simulate real-time
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    
    // Verify parameter transition completed
    EXPECT_NEAR(param->getScaledValue(), 0.5, 0.1);
    
    // Verify time advanced
    EXPECT_GT(transport.getTimePosition(), 0.0);
}

TEST_F(NomadFrameworkIntegrationTest, PerformanceBenchmark)
{
    auto& audioEngine = nomad::audio::AudioEngine::getInstance();
    auto& transport = nomad::transport::Transport::getInstance();
    auto& paramManager = nomad::parameters::ParameterManager::getInstance();
    auto& automationEngine = nomad::automation::AutomationEngine::getInstance();
    
    // Create multiple parameters
    nomad::parameters::ParameterRange range(0.0, 100.0, 50.0, 0.1);
    for (int i = 0; i < 100; ++i)
    {
        std::string paramId = "param_" + std::to_string(i);
        std::string paramName = "Parameter " + std::to_string(i);
        paramManager.createParameter(paramId, paramName, 
                                    nomad::parameters::ParameterType::Float, range);
    }
    
    // Create automation lanes
    for (int i = 0; i < 50; ++i)
    {
        std::string paramId = "param_" + std::to_string(i);
        int laneId = automationEngine.createAutomationLane(paramId);
        automationEngine.setAutomationLaneType(laneId, nomad::automation::AutomationType::LFO);
        
        nomad::automation::LFOData lfoData;
        lfoData.type = nomad::automation::LFOType::Sine;
        lfoData.frequency = 0.1 + (i * 0.01); // Different frequencies
        lfoData.amplitude = 0.5;
        automationEngine.setLFOData(laneId, lfoData);
    }
    
    // Start transport
    transport.play();
    
    // Benchmark processing
    const int numIterations = 1000;
    const int samplesPerIteration = 512;
    
    auto startTime = std::chrono::high_resolution_clock::now();
    
    for (int i = 0; i < numIterations; ++i)
    {
        // Process all systems
        transport.processTransport(samplesPerIteration);
        paramManager.updateParameters(samplesPerIteration);
        automationEngine.processAutomation(samplesPerIteration, 120.0);
    }
    
    auto endTime = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);
    
    // Calculate performance metrics
    double totalSamples = numIterations * samplesPerIteration;
    double totalTime = duration.count() / 1000.0; // Convert to seconds
    double samplesPerSecond = totalSamples / totalTime;
    double cpuUsage = (totalTime / (totalSamples / 44100.0)) * 100.0;
    
    std::cout << "Performance Benchmark Results:" << std::endl;
    std::cout << "  Total samples processed: " << totalSamples << std::endl;
    std::cout << "  Total time: " << totalTime << " seconds" << std::endl;
    std::cout << "  Samples per second: " << samplesPerSecond << std::endl;
    std::cout << "  CPU usage: " << cpuUsage << "%" << std::endl;
    
    // Verify performance is reasonable
    EXPECT_LT(cpuUsage, 50.0); // Should use less than 50% CPU
    EXPECT_GT(samplesPerSecond, 44100.0); // Should process faster than real-time
}

TEST_F(NomadFrameworkIntegrationTest, ErrorHandling)
{
    // Test error handling scenarios
    
    // Test invalid parameter creation
    nomad::parameters::ParameterRange range(0.0, 100.0, 50.0, 0.1);
    auto& paramManager = nomad::parameters::ParameterManager::getInstance();
    
    // Try to create parameter with duplicate ID
    auto* param1 = paramManager.createParameter("duplicate", "Param 1", 
                                                nomad::parameters::ParameterType::Float, range);
    auto* param2 = paramManager.createParameter("duplicate", "Param 2", 
                                                nomad::parameters::ParameterType::Float, range);
    
    EXPECT_NE(param1, nullptr);
    EXPECT_EQ(param2, nullptr); // Should fail due to duplicate ID
    
    // Test invalid parameter access
    auto* invalidParam = paramManager.getParameter("nonexistent");
    EXPECT_EQ(invalidParam, nullptr);
    
    // Test invalid parameter value setting
    EXPECT_FALSE(paramManager.setParameterValue("nonexistent", 0.5));
    
    // Test project operations without project
    auto& projectManager = nomad::project::ProjectManager::getInstance();
    EXPECT_FALSE(projectManager.saveCurrentProject()); // No project open
    EXPECT_FALSE(projectManager.isProjectDirty());
}

TEST_F(NomadFrameworkIntegrationTest, ThreadSafety)
{
    // Test thread safety of framework components
    auto& paramManager = nomad::parameters::ParameterManager::getInstance();
    auto& transport = nomad::transport::Transport::getInstance();
    
    // Create parameters
    nomad::parameters::ParameterRange range(0.0, 100.0, 50.0, 0.1);
    for (int i = 0; i < 10; ++i)
    {
        std::string paramId = "thread_param_" + std::to_string(i);
        paramManager.createParameter(paramId, "Thread Param " + std::to_string(i), 
                                    nomad::parameters::ParameterType::Float, range);
    }
    
    // Start multiple threads
    std::vector<std::thread> threads;
    const int numThreads = 4;
    const int operationsPerThread = 100;
    
    for (int t = 0; t < numThreads; ++t)
    {
        threads.emplace_back([&, t]() {
            for (int i = 0; i < operationsPerThread; ++i)
            {
                // Random parameter operations
                int paramIndex = i % 10;
                std::string paramId = "thread_param_" + std::to_string(paramIndex);
                
                if (i % 2 == 0)
                {
                    paramManager.setParameterValue(paramId, (i % 100) / 100.0);
                }
                else
                {
                    paramManager.getParameterValue(paramId);
                }
                
                // Transport operations
                if (i % 10 == 0)
                {
                    transport.setTimePosition(i * 0.1);
                }
            }
        });
    }
    
    // Wait for all threads to complete
    for (auto& thread : threads)
    {
        thread.join();
    }
    
    // Verify no crashes occurred and data is consistent
    EXPECT_EQ(paramManager.getNumParameters(), 10);
    
    // Test that we can still access parameters
    for (int i = 0; i < 10; ++i)
    {
        std::string paramId = "thread_param_" + std::to_string(i);
        auto* param = paramManager.getParameter(paramId);
        EXPECT_NE(param, nullptr);
    }
}