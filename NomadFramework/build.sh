#!/bin/bash

# Nomad Framework Build Script
# This script builds the framework and runs tests

set -e

echo "Building Nomad Framework..."

# Create build directory
mkdir -p build
cd build

# Configure with CMake
echo "Configuring with CMake..."
cmake .. -DCMAKE_BUILD_TYPE=Release

# Build the framework
echo "Building framework..."
make -j$(nproc)

# Build tests
echo "Building tests..."
make NomadFrameworkTests

# Build example
echo "Building example..."
make NomadExample

echo "Build completed successfully!"

# Run tests
echo "Running tests..."
./tests/NomadFrameworkTests

echo "All tests passed!"

# Run example
echo "Running example application..."
./examples/NomadExample

echo "Example completed successfully!"

echo "Nomad Framework build and test completed successfully!"