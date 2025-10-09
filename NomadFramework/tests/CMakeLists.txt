cmake_minimum_required(VERSION 3.15)

# Find testing framework
find_package(GTest REQUIRED)

# Include directories
include_directories(${CMAKE_SOURCE_DIR}/include)
include_directories(${CMAKE_SOURCE_DIR}/tests)

# Test source files
set(TEST_SOURCES
    TestMain.cpp
    TestAudioEngine.cpp
    TestMidiEngine.cpp
    TestTransport.cpp
    TestPluginHost.cpp
    TestParameterManager.cpp
    TestAutomationEngine.cpp
    TestProjectManager.cpp
    TestIntegration.cpp
)

# Create test executable
add_executable(NomadFrameworkTests ${TEST_SOURCES})

# Link libraries
target_link_libraries(NomadFrameworkTests 
    NomadFramework
    GTest::GTest
    GTest::Main
    ${JUCE_LIBRARIES}
)

# Add test to CTest
enable_testing()
add_test(NAME NomadFrameworkTests COMMAND NomadFrameworkTests)