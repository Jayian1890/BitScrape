#!/bin/bash

# Default build type
BUILD_TYPE="debug"
ENABLE_ASAN=false
BUILD_TESTS=true
CLEAN_BUILD=false

# Parse command line arguments
while [[ $# -gt 0 ]]; do
  case $1 in
    --release)
      BUILD_TYPE="release"
      shift
      ;;
    --debug)
      BUILD_TYPE="debug"
      shift
      ;;
    --asan)
      ENABLE_ASAN=true
      shift
      ;;
    --no-tests)
      BUILD_TESTS=false
      shift
      ;;
    --clean)
      CLEAN_BUILD=true
      shift
      ;;
    *)
      echo "Unknown option: $1"
      echo "Usage: $0 [--release|--debug] [--asan] [--no-tests] [--clean]"
      exit 1
      ;;
  esac
done

# Clean build directory if requested
if [ "$CLEAN_BUILD" = true ]; then
  echo "Cleaning build directory..."
  rm -rf build
fi

# Create build directory if it doesn't exist
mkdir -p build

# Configure Meson
echo "Configuring Meson build..."
meson setup build \
  --buildtype=$BUILD_TYPE \
  -Denable_asan=$ENABLE_ASAN \
  -Dbuild_tests=$BUILD_TESTS

# Build
echo "Building..."
meson compile -C build

# Run tests if enabled
if [ "$BUILD_TESTS" = true ]; then
  echo "Running tests..."
  meson test -C build
fi

echo "Build completed successfully!"