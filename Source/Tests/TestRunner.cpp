#include <JuceHeader.h>
#include "PatternTests.h"
#include <iostream>

/**
 * Test Runner
 * Runs all unit tests and reports results
 */
int main(int argc, char* argv[])
{
    // Create a unit test runner
    juce::UnitTestRunner runner;
    
    std::cout << "Running NOMAD Pattern Tests...\n";
    std::cout << "========================================\n\n";
    
    // Run all tests
    runner.runAllTests();
    
    // Print results
    for (int i = 0; i < runner.getNumResults(); ++i)
    {
        auto* result = runner.getResult(i);
        
        std::cout << "\n========================================\n";
        std::cout << "Test: " << result->unitTestName.toStdString() << "\n";
        std::cout << "========================================\n";
        std::cout << "Passes: " << result->passes << "\n";
        std::cout << "Failures: " << result->failures << "\n";
        
        if (result->failures > 0)
        {
            std::cout << "\nFailure messages:\n";
            for (const auto& message : result->messages)
            {
                std::cout << "  - " << message.toStdString() << "\n";
            }
        }
    }
    
    // Print summary
    int totalPasses = 0;
    int totalFailures = 0;
    
    for (int i = 0; i < runner.getNumResults(); ++i)
    {
        auto* result = runner.getResult(i);
        totalPasses += result->passes;
        totalFailures += result->failures;
    }
    
    std::cout << "\n========================================\n";
    std::cout << "SUMMARY\n";
    std::cout << "========================================\n";
    std::cout << "Total passes: " << totalPasses << "\n";
    std::cout << "Total failures: " << totalFailures << "\n";
    
    if (totalFailures == 0)
    {
        std::cout << "\n✓ All tests passed!\n";
    }
    else
    {
        std::cout << "\n✗ Some tests failed.\n";
    }
    
    std::cout << "========================================\n";
    
    // Return appropriate exit code
    return totalFailures == 0 ? 0 : 1;
}
