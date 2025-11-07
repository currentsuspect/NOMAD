// © 2025 Nomad Studios – All Rights Reserved. Licensed for personal & educational use only.
#include "Track.h"
#include "NomadLog.h"
#include <iostream>
#include <iomanip>
#include <cmath>
#include <vector>
#include <fstream>

using namespace Nomad::Audio;

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// Helper function to generate a test WAV file
bool generateTestWavFile(const std::string& filename, uint32_t sampleRate, double durationSeconds) {
    const uint16_t numChannels = 2;
    const uint16_t bitsPerSample = 16;
    const double frequency = 440.0;
    
    uint32_t numFrames = static_cast<uint32_t>(sampleRate * durationSeconds);
    uint32_t numSamples = numFrames * numChannels;
    
    std::vector<int16_t> audioData(numSamples);
    for (uint32_t frame = 0; frame < numFrames; ++frame) {
        double phase = 2.0 * M_PI * frequency * frame / sampleRate;
        int16_t sample = static_cast<int16_t>(0.5 * 32767.0 * std::sin(phase));
        audioData[frame * numChannels + 0] = sample;
        audioData[frame * numChannels + 1] = sample;
    }
    
    std::ofstream file(filename, std::ios::binary);
    if (!file) return false;
    
    uint32_t dataSize = numSamples * sizeof(int16_t);
    uint32_t fileSize = 36 + dataSize;
    uint32_t byteRate = sampleRate * numChannels * bitsPerSample / 8;
    uint16_t blockAlign = numChannels * bitsPerSample / 8;
    
    file.write("RIFF", 4);
    file.write(reinterpret_cast<const char*>(&fileSize), 4);
    file.write("WAVE", 4);
    file.write("fmt ", 4);
    uint32_t fmtSize = 16;
    file.write(reinterpret_cast<const char*>(&fmtSize), 4);
    uint16_t audioFormat = 1;
    file.write(reinterpret_cast<const char*>(&audioFormat), 2);
    file.write(reinterpret_cast<const char*>(&numChannels), 2);
    file.write(reinterpret_cast<const char*>(&sampleRate), 4);
    file.write(reinterpret_cast<const char*>(&byteRate), 4);
    file.write(reinterpret_cast<const char*>(&blockAlign), 2);
    file.write(reinterpret_cast<const char*>(&bitsPerSample), 2);
    file.write("data", 4);
    file.write(reinterpret_cast<const char*>(&dataSize), 4);
    file.write(reinterpret_cast<const char*>(audioData.data()), dataSize);
    
    file.close();
    return true;
}

int main() {
    std::cout << "========================================" << std::endl;
    std::cout << "  Seek While Stopped Test" << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << std::endl;
    
    // Generate test file
    const double testDuration = 5.0;
    const uint32_t sampleRate = 48000;
    
    std::cout << "Generating test audio file..." << std::endl;
    if (!generateTestWavFile("test_seek_stopped.wav", sampleRate, testDuration)) {
        std::cerr << "Failed to generate test file" << std::endl;
        return 1;
    }
    std::cout << "Generated: test_seek_stopped.wav (" << sampleRate << " Hz, " << testDuration << "s)" << std::endl;
    std::cout << std::endl;
    
    // Create track and load audio
    Track track("Test Track", 1);
    if (!track.loadAudioFile("test_seek_stopped.wav")) {
        std::cerr << "Failed to load test file" << std::endl;
        return 1;
    }
    
    track.setOutputSampleRate(sampleRate);
    
    int totalTests = 0;
    int passedTests = 0;
    
    // Test 1: Seek while stopped, then play
    std::cout << "Test 1: Seek to 2.5s while stopped, then play" << std::endl;
    std::cout << "-----------------------------------------------" << std::endl;
    {
        totalTests++;
        
        // Ensure track is stopped
        track.stop();
        
        // Seek to middle of track
        double seekPosition = 2.5;
        track.setPosition(seekPosition);
        
        double positionAfterSeek = track.getPosition();
        std::cout << "Position after seek: " << std::fixed << std::setprecision(3) << positionAfterSeek << "s" << std::endl;
        
        if (std::abs(positionAfterSeek - seekPosition) > 0.001) {
            std::cout << "FAIL: Position not set correctly after seek" << std::endl;
        } else {
            // Now play
            track.play();
            
            // Process a small amount of audio
            const uint32_t framesToProcess = 1024;
            std::vector<float> outputBuffer(framesToProcess * 2, 0.0f);
            track.processAudio(outputBuffer.data(), framesToProcess, 0.0);
            
            double positionAfterPlay = track.getPosition();
            double expectedPosition = seekPosition + (static_cast<double>(framesToProcess) / sampleRate);
            
            std::cout << "Position after playing " << framesToProcess << " frames: " << positionAfterPlay << "s" << std::endl;
            std::cout << "Expected position: " << expectedPosition << "s" << std::endl;
            
            double error = std::abs(positionAfterPlay - expectedPosition);
            std::cout << "Error: " << error << "s" << std::endl;
            
            if (error < 0.001) {
                std::cout << "PASS: Audio played from correct position" << std::endl;
                passedTests++;
            } else {
                std::cout << "FAIL: Audio did not play from seek position" << std::endl;
            }
        }
        
        track.stop();
    }
    
    std::cout << std::endl;
    
    // Test 2: Multiple seeks while stopped
    std::cout << "Test 2: Multiple seeks while stopped" << std::endl;
    std::cout << "--------------------------------------" << std::endl;
    {
        totalTests++;
        
        track.stop();
        
        std::vector<double> seekPositions = {1.0, 3.0, 0.5, 4.5};
        bool allPassed = true;
        
        for (double seekPos : seekPositions) {
            track.setPosition(seekPos);
            double actualPos = track.getPosition();
            
            std::cout << "Seek to " << seekPos << "s -> actual: " << actualPos << "s";
            
            if (std::abs(actualPos - seekPos) > 0.001) {
                std::cout << " FAIL" << std::endl;
                allPassed = false;
            } else {
                std::cout << " OK" << std::endl;
            }
        }
        
        if (allPassed) {
            std::cout << "PASS: All seeks while stopped worked correctly" << std::endl;
            passedTests++;
        } else {
            std::cout << "FAIL: Some seeks did not work correctly" << std::endl;
        }
    }
    
    std::cout << std::endl;
    
    // Test 3: Seek, play, stop, seek again, play
    std::cout << "Test 3: Seek -> Play -> Stop -> Seek -> Play" << std::endl;
    std::cout << "----------------------------------------------" << std::endl;
    {
        totalTests++;
        
        // First seek and play
        track.stop();
        track.setPosition(1.0);
        track.play();
        
        std::vector<float> outputBuffer(512 * 2, 0.0f);
        track.processAudio(outputBuffer.data(), 512, 0.0);
        
        // Stop
        track.stop();
        double positionAfterStop = track.getPosition();
        std::cout << "Position after stop: " << positionAfterStop << "s (should be 0.0)" << std::endl;
        
        // Second seek and play
        track.setPosition(3.0);
        double positionAfterSecondSeek = track.getPosition();
        std::cout << "Position after second seek: " << positionAfterSecondSeek << "s" << std::endl;
        
        track.play();
        track.processAudio(outputBuffer.data(), 512, 0.0);
        
        double finalPosition = track.getPosition();
        double expectedFinal = 3.0 + (512.0 / sampleRate);
        
        std::cout << "Final position: " << finalPosition << "s" << std::endl;
        std::cout << "Expected: " << expectedFinal << "s" << std::endl;
        
        double error = std::abs(finalPosition - expectedFinal);
        if (error < 0.001) {
            std::cout << "PASS: Second seek and play worked correctly" << std::endl;
            passedTests++;
        } else {
            std::cout << "FAIL: Second seek did not work correctly" << std::endl;
        }
        
        track.stop();
    }
    
    // Summary
    std::cout << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << "  Test Summary" << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << "Total tests: " << totalTests << std::endl;
    std::cout << "Passed: " << passedTests << std::endl;
    std::cout << "Failed: " << (totalTests - passedTests) << std::endl;
    std::cout << "Success rate: " << std::fixed << std::setprecision(1) 
              << (totalTests > 0 ? (passedTests * 100.0 / totalTests) : 0.0) << "%" << std::endl;
    std::cout << "========================================" << std::endl;
    
    return (passedTests == totalTests) ? 0 : 1;
}
