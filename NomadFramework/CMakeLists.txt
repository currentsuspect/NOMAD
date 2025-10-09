cmake_minimum_required(VERSION 3.15)
project(NomadFramework VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find JUCE
find_package(PkgConfig REQUIRED)
pkg_check_modules(JUCE REQUIRED juce)

# Include directories
include_directories(include)
include_directories(${JUCE_INCLUDE_DIRS})

# Compiler flags
add_compile_options(${JUCE_CFLAGS})
add_compile_definitions(${JUCE_CFLAGS_OTHER})

# Link directories
link_directories(${JUCE_LIBRARY_DIRS})

# Source files
file(GLOB_RECURSE NOMAD_SOURCES 
    "src/*.cpp"
    "include/*.h"
)

# Create the library
add_library(NomadFramework STATIC ${NOMAD_SOURCES})

# Link libraries
target_link_libraries(NomadFramework ${JUCE_LIBRARIES})

# Example application
add_executable(NomadExample examples/NomadExample.cpp)
target_link_libraries(NomadExample NomadFramework)

# Tests
enable_testing()
add_subdirectory(tests)