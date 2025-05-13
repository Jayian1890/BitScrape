# BitScrape Build System

This document describes how to build the BitScrape project using CMake.

## Prerequisites

- CMake 3.14 or higher
- C++20 compatible compiler
- Git (for fetching dependencies)

## Building the Project

### Debug Build (CLI Only)

To build the project in debug mode targeting only the CLI application:

```bash
# Using the provided script
./build_debug.sh

# Or manually
mkdir -p build_debug
cd build_debug
cmake -DCMAKE_BUILD_TYPE=Debug ..
cmake --build .
```

The executable will be located at `build_debug/apps/cli/bitscrape_cli`.

### Running the Application

```bash
# From the build directory
cd build_debug
./apps/cli/bitscrape_cli
```

## Project Structure

- `apps/cli`: CLI application
- `modules/`: Core modules
  - `beacon`: Logging and notification system
  - `bencode`: Bencode encoding/decoding
  - `bittorrent`: BitTorrent protocol implementation
  - `core`: Core application logic
  - `dht`: DHT protocol implementation
  - `event`: Event system
  - `network`: Network communication
  - `storage`: Data storage
  - `tracker`: Tracker communication
  - `types`: Common types
  - `web`: Web interface

## Testing

Tests are built automatically if they exist. To run the tests:

```bash
cd build_debug
ctest
```

## Configuration

The application uses a configuration file `bitscrape.conf` which is copied to the build directory during the build process.

## Customizing the Build

You can customize the build by modifying the CMake options:

```bash
# Example: Build with different compiler
cmake -DCMAKE_CXX_COMPILER=clang++ -DCMAKE_BUILD_TYPE=Debug ..

# Example: Build with additional compiler flags
cmake -DCMAKE_CXX_FLAGS="-fsanitize=address" -DCMAKE_BUILD_TYPE=Debug ..
```
