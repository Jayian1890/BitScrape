# BitScrape Build System

This document describes how to build the BitScrape project. The repository now includes Makefiles to build the project (recommended). CMake-based instructions are preserved below for reference.

## Makefile build (recommended)

- Build everything: `make` (runs modules and apps builds)
- Build with debug flags: `make DEBUG=1`
- Clean: `make clean`
- Install: `make install PREFIX=/usr/local`
- Build a single module: `make -C modules/<module>`
- Build CLI only: `make -C apps/cli`
- Tests: unit test targets are module-scoped and require GoogleTest to be available; see the "Testing" section below for options.


## Prerequisites

- CMake 3.14 or higher
- C++23 compatible compiler
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

- Makefile-based tests (preferred): Install GoogleTest and run tests with the new Makefiles. For macOS: `brew install googletest`.

  - Run all module tests from the repo root:

    ```bash
    make test
    ```

  - Run a single module's tests:

    ```bash
    make -C modules/<module> test
    ```

  - Tests are built with the vendored doctest header; no external GoogleTest installation is required.

- CMake-based tests (legacy): The previous CMake flow is still available; build with CMake and run `ctest` as shown below.

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
