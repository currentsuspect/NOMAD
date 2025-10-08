# NOMAD Unit Tests

This directory contains unit tests for the NOMAD DAW project.

## Running Tests

### Windows

```bash
# Build the tests
cmake --build build --config Debug --target NOMAD_Tests

# Run the tests
.\build\NOMAD_Tests_artefacts\Debug\NOMAD_Tests.exe
```

### Using CTest

```bash
cd build
ctest -C Debug
```

## Test Coverage

### PatternTests.h

Tests for the Pattern class (Requirements 1.2, 1.3, 1.4, 1.5):

- **Note Addition**: Tests adding single and multiple notes, updating existing notes
- **Note Removal**: Tests removing notes by step/track and by step/track/pitch, clearing all notes
- **Range Queries**: Tests querying notes within specific step ranges
- **Pattern Copy/Paste**: Tests deep copying via `clone()` and `copyFrom()` methods
- **Pattern Length**: Tests setting pattern length and automatic note removal beyond new length
- **Note Validation**: Tests validation of note parameters (step, pitch, velocity)

## Test Results

All tests should pass with 0 failures. The test runner will output:
- Individual test results with pass/fail counts
- Summary of total passes and failures
- Exit code 0 for success, 1 for failure

## Adding New Tests

To add new tests:

1. Create a new test class inheriting from `juce::UnitTest`
2. Implement the `runTest()` method with test cases
3. Create a static instance of your test class
4. Include the test header in `TestRunner.cpp`
5. Rebuild and run

Example:

```cpp
class MyNewTests : public juce::UnitTest
{
public:
    MyNewTests() : juce::UnitTest("My New Tests", "Category") {}
    
    void runTest() override
    {
        beginTest("Test Case Name");
        expect(condition, "Failure message");
    }
};

static MyNewTests myNewTests;
```
