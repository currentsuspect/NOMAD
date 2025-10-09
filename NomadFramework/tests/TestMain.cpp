/**
 * @file TestMain.cpp
 * @brief Main test file for Nomad Framework
 */

#include <gtest/gtest.h>
#include <JuceHeader.h>

int main(int argc, char** argv)
{
    // Initialize JUCE
    juce::ScopedJuceInitialiser_GUI juceInit;
    
    // Initialize Google Test
    ::testing::InitGoogleTest(&argc, argv);
    
    // Run tests
    return RUN_ALL_TESTS();
}