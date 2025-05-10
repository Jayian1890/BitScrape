#!/bin/bash

# Default values
BUILD_TYPE="Debug"
ENABLE_ASAN=OFF
BUILD_TESTS=ON

# Parse command line arguments
while [[ $# -gt 0 ]]; do
    case $1 in
        --release)
            BUILD_TYPE="Release"
            shift
            ;;
        --debug)
            BUILD_TYPE="Debug"
            shift
            ;;
        --asan)
            ENABLE_ASAN=ON
            shift
            ;;
        --no-tests)
            BUILD_TESTS=OFF
            shift
            ;;
        *)
            echo "Unknown option: $1"
            exit 1
            ;;
    esac
done

# Create build directory if it doesn't exist
mkdir -p build

# Configure CMake
echo "Configuring CMake build..."
cmake -S . -B build \
    -DCMAKE_BUILD_TYPE=$BUILD_TYPE \
    -DENABLE_ASAN=$ENABLE_ASAN \
    -DBUILD_TESTS=$BUILD_TESTS

# Build
echo "Building..."
# Determine the number of CPU cores for parallel build
if [ "$(uname)" = "Darwin" ]; then
    # macOS
    NUM_CORES=$(sysctl -n hw.ncpu)
else
    # Linux and others with nproc
    NUM_CORES=$(nproc 2>/dev/null || echo 4)
fi
cmake --build build --config $BUILD_TYPE -j$NUM_CORES

# Run tests if enabled
if [ "$BUILD_TESTS" = ON ]; then
    echo "Running tests..."
    cd build && ctest --output-on-failure
fi

echo "Build completed successfully!"
