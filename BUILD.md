# BitScrape Build System

This document describes how to build the BitScrape project. The repository uses Makefiles exclusively for building and testing.

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

```
bitscrape/
├── apps/
│   └── cli/              # CLI application
├── include/
│   └── bitscrape/        # Centralized public headers
│       ├── beacon/
│       ├── bencode/
│       ├── bittorrent/
│       ├── core/
│       ├── dht/
│       ├── event/
│       ├── lock/
│       ├── network/
│       ├── storage/
│       ├── tracker/
│       ├── types/
│       └── web/
├── modules/              # Module implementations
│   ├── beacon/           # Logging and notification system
│   ├── bencode/          # Bencode encoding/decoding
│   ├── bittorrent/       # BitTorrent protocol implementation
│   ├── core/             # Core application logic
│   ├── dht/              # DHT protocol implementation
│   ├── event/            # Event system
│   ├── lock/             # Locking utilities
│   ├── network/          # Network communication
│   ├── storage/          # Data storage
│   ├── tracker/          # Tracker communication
│   ├── types/            # Common types
│   └── web/              # Web interface
├── tests/                # Unit tests
│   ├── helpers/          # Test helper utilities
│   ├── core/             # Tests for core module
│   └── doctest_main.cpp  # Shared test runner
└── build/                # Build output directory
```

### Module Layout

Each module under `modules/` contains:
- `src/` - Implementation files (`.cpp`)
- `Makefile` - Module-specific build rules (includes `module.mk`)

Public headers for all modules are centralized under `include/bitscrape/<module>/`.

### Test Layout

Tests are organized under `tests/<module>/`:
- `tests/core/configuration_tests.cpp`
- `tests/core/controller_tests.cpp`

Test helpers live in `tests/helpers/`.

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

- Run a specific test binary directly:

  ```bash
  ./build/tests/<module>/run_tests
  ```

### Coverage

- Install `gcovr` once (Python package): `pip install gcovr`
- Generate coverage and an `lcov.info` file (used by VS Code Coverage Gutters):

  ```bash
  make coverage
  ```

The coverage target rebuilds everything with instrumentation, runs all module tests, and writes `lcov.info` in the repo root.

## Configuration

The Make build produces a `build/bitscrape.json.template` file plus `build/public` and `build/data` directories that mirror the previous CMake-generated assets. Copy or edit the template as needed for runtime configuration.

## Customizing the Build

Override standard Make variables as needed:

```bash
make CXX=clang++ CXXFLAGS="-std=c++23 -O2 -Wall"
make DEBUG=1
make PREFIX=/opt/bitscrape install
```

## Adding New Code

### Adding a New Header

Place new public headers under `include/bitscrape/<module>/`:

```
include/bitscrape/core/my_new_header.hpp
```

### Adding a New Implementation File

Place new implementation files under `modules/<module>/src/`:

```
modules/core/src/my_new_impl.cpp
```

### Adding a New Test

Place new test files under `tests/<module>/`:

```
tests/core/my_new_tests.cpp
```

Include test helpers with:
```cpp
#include "test_helpers.hpp"
```
