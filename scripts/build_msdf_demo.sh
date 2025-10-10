#!/bin/bash
echo "Building MSDF Text Demo..."

# Create build directory
mkdir -p build
cd build

# Configure with CMake
cmake .. -DCMAKE_BUILD_TYPE=Release
if [ $? -ne 0 ]; then
    echo "CMake configuration failed"
    exit 1
fi

# Build the demo
make TEXT_DEMO -j$(nproc)
if [ $? -ne 0 ]; then
    echo "Build failed"
    exit 1
fi

echo "Build completed successfully!"
echo "Run: ./TEXT_DEMO"