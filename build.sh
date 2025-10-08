#!/bin/bash
# Build script for NOMAD DAW on Linux

echo "Building NOMAD DAW..."
echo

# Create build directory if it doesn't exist
mkdir -p build

# Navigate to build directory
cd build

# Configure with CMake
echo "Configuring project with CMake..."
cmake .. -DCMAKE_BUILD_TYPE=Release
if [ $? -ne 0 ]; then
    echo "CMake configuration failed!"
    cd ..
    exit 1
fi

echo
echo "Building project..."
make -j$(nproc)
if [ $? -ne 0 ]; then
    echo "Build failed!"
    cd ..
    exit 1
fi

echo
echo "Build successful!"
echo "Executable: build/NOMAD_artefacts/Release/NOMAD"

cd ..
