# BitScrape Build System

This document describes how to build the BitScrape project. The repository now uses Makefiles exclusively for building and testing.

## Makefile build

- Build everything: `make` (runs modules and apps builds)
- Build with debug flags: `make DEBUG=1`
- Clean: `make clean`
- Install: `make install PREFIX=/usr/local`
- Build a single module: `make -C modules/<module>`
- Build CLI only: `make -C apps/cli`
- Tests: `make test` for all modules or `make -C modules/<module> test` for one module.


## Prerequisites

- C++23 compatible compiler
- make
- Git (for fetching dependencies)

## Building the Project

### Debug Build (CLI Only)

```bash
make DEBUG=1 -C modules
make DEBUG=1 -C apps/cli
```

The executable will be located at `build/bin/bitscrape_cli`.

### Running the Application

```bash
./build/bin/bitscrape_cli
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

- Tests are built with the vendored doctest header; no external GoogleTest/GMock installation is required.
- Run all module tests from the repo root:

  ```bash
  make test
  ```

- Run a single module's tests:

  ```bash
  make -C modules/<module> test
  ```

## Configuration

The Make build produces a `build/bitscrape.json.template` file plus `build/public` and `build/data` directories that mirror the previous CMake-generated assets. Copy or edit the template as needed for runtime configuration.

## Customizing the Build

Override standard Make variables as needed:

```bash
make CXX=clang++ CXXFLAGS="-std=c++23 -O2 -Wall" 
make DEBUG=1
make PREFIX=/opt/bitscrape install
```
