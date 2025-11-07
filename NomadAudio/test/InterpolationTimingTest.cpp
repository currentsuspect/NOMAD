// © 2025 Nomad Studios – All Rights Reserved. Licensed for personal & educational use only.
#include "Track.h"
#include "TrackManager.h"
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

// Helper function to generate a test WAV file with a specific sample rate
bool generateTestWavFile(const std::string& filename, uint32_t sampleRate, double durationSeconds, double frequency) {
    const uint16_t numChannels = 2;
    const uint16_t bitsPerSample = 16;
    
    uint32_t numFrames = static_cast<uint32_t>(sampleRate * durationSeconds);
    uint32_t numSamples = numFrames * numChannels;
    
    // Generate sine wave data
    std::vector<int16_t> audioData(numSamples);
    for (uint32_t frame = 0; frame < numFrames; ++frame) {
        double phase = 2.0 * M_PI * frequency * frame / sampleRate;
        int16_t sample = static_cast<int16_t>(0.5 * 32767.0 * std::sin(phase));
        audioData[frame * numChannels + 0] = sample; // Left
        audioData[frame * numChannels + 1] = sample; // Right
    }
    
    // Write WAV file
    std::ofstream file(filename, std::ios::binary);
    if (!file) {
        std::cerr << "Failed to create test file: " << filename << std::endl;
        return false;
    }
    
    // WAV header
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
    uint16_t audioFormat = 1; // PCM
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
    std::cout << "Generated test file: " << filename << " (" << sampleRate << " Hz, " 
              << durationSeconds << "s, " << frequency << " Hz tone)" << std::endl;
    return true;
}

// Test helper to verify playhead position accuracy
bool testPlayheadAccuracy(Track& track, uint32_t outputSampleRate, uint32_t numFramesToProcess, double expectedDuration) {
    std::cout << "\n--- Testing Playhead Accuracy ---" << std::endl;
    std::cout << "Output sample rate: " << outputSampleRate << " Hz" << std::endl;
    std::cout << "Processing " << numFramesToProcess << " frames" << std::endl;
    
    // Set output sample rate
    track.setOutputSampleRate(outputSampleRate);
    
    // Start playback
    track.play();
    
    // Process audio in chunks
    const uint32_t bufferSize = 512;
    std::vector<float> outputBuffer(bufferSize * 2, 0.0f); // Stereo
    uint32_t framesProcessed = 0;
    
    while (framesProcessed < numFramesToProcess) {
        uint32_t framesToProcess = std::min(bufferSize, numFramesToProcess - framesProcessed);
        std::fill(outputBuffer.begin(), outputBuffer.end(), 0.0f);
        
        track.processAudio(outputBuffer.data(), framesToProcess, 0.0);
        framesProcessed += framesToProcess;
    }
    
    // Get final position
    double actualPosition = track.getPosition();
    double expectedPosition = static_cast<double>(numFramesToProcess) / outputSampleRate;
    
    std::cout << "Expected position: " << std::fixed << std::setprecision(6) << expectedPosition << " seconds" << std::endl;
    std::cout << "Actual position:   " << std::fixed << std::setprecision(6) << actualPosition << " seconds" << std::endl;
    
    // Calculate error
    double error = std::abs(actualPosition - expectedPosition);
    double errorPercent = (error / expectedPosition) * 100.0;
    
    std::cout << "Error: " << std::fixed << std::setprecision(6) << error << " seconds (" 
              << std::setprecision(3) << errorPercent << "%)" << std::endl;
    
    // Allow 0.1% error tolerance (very tight)
    bool passed = errorPercent < 0.1;
    std::cout << "Result: " << (passed ? "PASS" : "FAIL") << std::endl;
    
    track.stop();
    return passed;
}

// Test seeking functionality
bool testSeekingAccuracy(Track& track, uint32_t outputSampleRate) {
    std::cout << "\n--- Testing Seeking Accuracy ---" << std::endl;
    
    track.setOutputSampleRate(outputSampleRate);
    double duration = track.getDuration();
    
    // Test seeking to various positions
    std::vector<double> testPositions = {0.0, duration * 0.25, duration * 0.5, duration * 0.75, duration};
    
    bool allPassed = true;
    for (double targetPosition : testPositions) {
        track.setPosition(targetPosition);
        double actualPosition = track.getPosition();
        double error = std::abs(actualPosition - targetPosition);
        
        std::cout << "Seek to " << std::fixed << std::setprecision(3) << targetPosition 
                  << "s -> actual: " << actualPosition << "s (error: " << error << "s)" << std::endl;
        
        // Allow 1ms error tolerance for seeking
        if (error > 0.001) {
            std::cout << "  FAIL: Seek error too large" << std::endl;
            allPassed = false;
        }
    }
    
    std::cout << "Seeking test: " << (allPassed ? "PASS" : "FAIL") << std::endl;
    return allPassed;
}

// Test continuous playback without drift
bool testContinuousPlayback(Track& track, uint32_t outputSampleRate) {
    std::cout << "\n--- Testing Continuous Playback (No Drift) ---" << std::endl;
    
    track.setOutputSampleRate(outputSampleRate);
    track.play();
    
    double duration = track.getDuration();
    
    // Process through entire duration multiple times by seeking back
    const uint32_t bufferSize = 512;
    std::vector<float> outputBuffer(bufferSize * 2, 0.0f);
    
    std::vector<double> endPositions;
    
    for (int iteration = 0; iteration < 3; ++iteration) {
        track.setPosition(0.0);
        track.play();
        
        uint32_t totalFrames = static_cast<uint32_t>(duration * outputSampleRate);
        uint32_t framesProcessed = 0;
        
        while (framesProcessed < totalFrames) {
            uint32_t framesToProcess = std::min(bufferSize, totalFrames - framesProcessed);
            std::fill(outputBuffer.begin(), outputBuffer.end(), 0.0f);
            
            track.processAudio(outputBuffer.data(), framesToProcess, 0.0);
            framesProcessed += framesToProcess;
        }
        
        double position = track.getPosition();
        endPositions.push_back(position);
        std::cout << "Iteration " << (iteration + 1) << " end position: " << std::fixed << std::setprecision(6) 
                  << position << " seconds" << std::endl;
    }
    
    // Check for drift between iterations
    bool passed = true;
    for (size_t i = 1; i < endPositions.size(); ++i) {
        double drift = std::abs(endPositions[i] - endPositions[0]);
        if (drift > 0.001) { // Allow 1ms drift tolerance
            std::cout << "FAIL: Drift detected: " << drift << " seconds" << std::endl;
            passed = false;
        }
    }
    
    track.stop();
    
    std::cout << "Continuous playback test: " << (passed ? "PASS" : "FAIL") << std::endl;
    return passed;
}

int main() {
    std::cout << "========================================" << std::endl;
    std::cout << "  Interpolation Timing Fix Test Suite" << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << std::endl;
    
    // Generate test files with different sample rates
    const double testDuration = 2.0; // 2 seconds
    const double testFrequency = 440.0; // A4 note
    
    std::cout << "Generating test audio files..." << std::endl;
    generateTestWavFile("test_44100.wav", 44100, testDuration, testFrequency);
    generateTestWavFile("test_48000.wav", 48000, testDuration, testFrequency);
    generateTestWavFile("test_96000.wav", 96000, testDuration, testFrequency);
    
    int totalTests = 0;
    int passedTests = 0;
    
    // Test 3.1: 44.1kHz audio on 48kHz output device
    std::cout << "\n========================================" << std::endl;
    std::cout << "Test 3.1: 44.1kHz audio on 48kHz output" << std::endl;
    std::cout << "========================================" << std::endl;
    {
        Track track("Test Track 44.1k", 1);
        if (track.loadAudioFile("test_44100.wav")) {
            totalTests++;
            if (testPlayheadAccuracy(track, 48000, 48000, testDuration)) {
                passedTests++;
            }
            
            totalTests++;
            if (testSeekingAccuracy(track, 48000)) {
                passedTests++;
            }
        } else {
            std::cerr << "Failed to load test file" << std::endl;
        }
    }
    
    // Test 3.2: 48kHz audio on 44.1kHz output device
    std::cout << "\n========================================" << std::endl;
    std::cout << "Test 3.2: 48kHz audio on 44.1kHz output" << std::endl;
    std::cout << "========================================" << std::endl;
    {
        Track track("Test Track 48k", 2);
        if (track.loadAudioFile("test_48000.wav")) {
            totalTests++;
            if (testPlayheadAccuracy(track, 44100, 44100, testDuration)) {
                passedTests++;
            }
            
            totalTests++;
            if (testSeekingAccuracy(track, 44100)) {
                passedTests++;
            }
        } else {
            std::cerr << "Failed to load test file" << std::endl;
        }
    }
    
    // Test 3.3: 96kHz audio on 48kHz output device
    std::cout << "\n========================================" << std::endl;
    std::cout << "Test 3.3: 96kHz audio on 48kHz output" << std::endl;
    std::cout << "========================================" << std::endl;
    {
        Track track("Test Track 96k", 3);
        if (track.loadAudioFile("test_96000.wav")) {
            totalTests++;
            if (testPlayheadAccuracy(track, 48000, 48000, testDuration)) {
                passedTests++;
            }
            
            totalTests++;
            if (testSeekingAccuracy(track, 48000)) {
                passedTests++;
            }
        } else {
            std::cerr << "Failed to load test file" << std::endl;
        }
    }
    
    // Test 3.5: Continuous playback without drift (using 44.1k -> 48k as example)
    std::cout << "\n========================================" << std::endl;
    std::cout << "Test 3.5: Continuous playback (no drift)" << std::endl;
    std::cout << "========================================" << std::endl;
    {
        Track track("Test Track Continuous", 4);
        if (track.loadAudioFile("test_44100.wav")) {
            totalTests++;
            if (testContinuousPlayback(track, 48000)) {
                passedTests++;
            }
        } else {
            std::cerr << "Failed to load test file" << std::endl;
        }
    }
    
    // Summary
    std::cout << "\n========================================" << std::endl;
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
