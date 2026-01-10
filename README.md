# BitScrape

BitScrape is a high-performance BitTorrent DHT crawler and metadata scraper designed to efficiently discover and collect metadata from the BitTorrent distributed hash table network.

[![License: MIT](https://img.shields.io/badge/License-MIT-blue.svg)](https://opensource.org/licenses/MIT)

## Features

- **DHT Crawling**: Discover nodes and infohashes in the BitTorrent DHT network
- **Metadata Acquisition**: Download and parse BitTorrent metadata from peers
- **High Performance**: Handle thousands of concurrent connections efficiently
- **Event-Driven Architecture**: Non-blocking operations using modern C++23 features
- **Modular Design**: Independent components that can be used separately
- **Cross-Platform**: Runs on macOS, Linux, and Windows

## Architecture

BitScrape follows a modular, event-driven architecture designed to handle high-throughput network operations efficiently:

```
┌─────────────────────────────────────────────────────────────────┐
│                        Application Layer                         │
│  ┌─────────────┐  ┌─────────────┐  ┌─────────────┐  ┌─────────┐ │
│  │ CLI App     │  │ GUI App     │  │ Web UI      │  │ API     │ │
│  └─────────────┘  └─────────────┘  └─────────────┘  └─────────┘ │
└─────────────────────────────────────────────────────────────────┘
                              │
┌─────────────────────────────────────────────────────────────────┐
│                         Core Layer                               │
│  ┌─────────────┐  ┌─────────────┐  ┌─────────────┐  ┌─────────┐ │
│  │ Controller  │  │ Config      │  │ Storage     │  │ Stats   │ │
│  └─────────────┘  └─────────────┘  └─────────────┘  └─────────┘ │
└─────────────────────────────────────────────────────────────────┘
                              │
┌─────────────────────────────────────────────────────────────────┐
│                       Protocol Layer                             │
│  ┌─────────────┐  ┌─────────────┐  ┌─────────────┐              │
│  │ DHT         │  │ BitTorrent  │  │ Tracker     │              │
│  └─────────────┘  └─────────────┘  └─────────────┘              │
└─────────────────────────────────────────────────────────────────┘
                              │
┌─────────────────────────────────────────────────────────────────┐
│                      Foundation Layer                            │
│  ┌─────────────┐  ┌─────────────┐  ┌─────────────┐              │
│  │ Network     │  │ Bencode     │  │ Event       │              │
│  └─────────────┘  └─────────────┘  └─────────────┘              │
└─────────────────────────────────────────────────────────────────┘
```

Key architectural principles:

- **Modularity**: Each component is independent and reusable
- **Event-Driven**: Components communicate through events rather than direct method calls
- **Asynchronous**: Network operations are performed asynchronously to prevent blocking
- **Type-Safe**: Strong typing is used throughout the codebase to prevent errors

## Requirements

- C++23 compatible compiler (GCC 11+, Clang 14+, or MSVC 19.29+)
- make

## Building (Make-only)

The repository ships Makefiles for modules and the CLI. Common tasks:

```bash
# Build everything (modules + apps)
make

# Build with debug flags (adds -g -O0)
make DEBUG=1

# Build a single module
make -C modules/<module>

# Build the CLI app only
make -C apps/cli

# Clean
make clean

# Install
make install PREFIX=/usr/local

# Run all module tests
make test

# Run tests for one module
make -C modules/<module> test
```

Tests use the vendored doctest header; no external GoogleTest/GMock or CMake toolchain is required.

## Usage

### Command Line Interface

```bash
# Start crawling the DHT network
bitscrape-cli --bootstrap-nodes=router.bittorrent.com:6881,router.utorrent.com:6881

# Crawl with specific options
bitscrape-cli --max-connections=1000 --storage-path=/path/to/storage

# Show help
bitscrape-cli --help
```

### Web Interface

The web interface is available at http://localhost:8080 when running the web UI:

```bash
bitscrape-web --port=8080
```

## Project Structure

```
bitscrape/
├── include/                  # Centralized public headers
│   └── bitscrape/
│       ├── beacon/           # Logging headers
│       ├── bencode/          # Bencode headers
│       ├── bittorrent/       # BitTorrent headers
│       ├── core/             # Core headers
│       ├── dht/              # DHT headers
│       ├── event/            # Event system headers
│       ├── lock/             # Lock utility headers
│       ├── network/          # Network headers
│       ├── storage/          # Storage headers
│       ├── tracker/          # Tracker headers
│       ├── types/            # Type definition headers
│       └── web/              # Web interface headers
├── modules/                  # Module implementations
│   ├── beacon/src/           # Logging implementation
│   ├── bencode/src/          # Bencode encoding/decoding
│   ├── bittorrent/src/       # BitTorrent protocol
│   ├── core/src/             # Core application logic
│   ├── dht/src/              # DHT protocol
│   ├── event/src/            # Event system
│   ├── lock/src/             # Lock utilities
│   ├── network/src/          # Network abstraction
│   ├── storage/src/          # Data storage
│   ├── tracker/src/          # Tracker communication
│   ├── types/src/            # Type implementations
│   └── web/src/              # Web interface
├── apps/                     # Applications
│   └── cli/                  # Command-line interface
├── tests/                    # Unit tests
│   ├── helpers/              # Test utilities
│   ├── core/                 # Core module tests
│   └── doctest_main.cpp      # Shared test runner
├── scripts/                  # Utility scripts
└── Makefile                  # Top-level Make entry point
```

## Module Overview

### Foundation Layer

- **Types**: Core type definitions (NodeID, InfoHash, Endpoint, etc.)
- **Event**: Event system for communication between components
- **Network**: Network abstraction layer for UDP, TCP, and HTTP
- **Bencode**: Bencode encoding/decoding

### Protocol Layer

- **DHT**: DHT protocol implementation
- **BitTorrent**: BitTorrent protocol implementation (includes metadata handling)
- **Tracker**: Tracker communication

### Core Layer

- **Controller**: Application controller
- **Storage**: Data storage
- **Stats**: Statistics collection and reporting

### Application Layer

- **CLI**: Command-line interface
- **GUI**: Graphical user interface
- **Web UI**: Web interface
- **API**: API for external applications

## Contributing

Contributions are welcome! Please feel free to submit a Pull Request.

1. Fork the repository
2. Create your feature branch (`git checkout -b feature/amazing-feature`)
3. Commit your changes (`git commit -m 'Add some amazing feature'`)
4. Push to the branch (`git push origin feature/amazing-feature`)
5. Open a Pull Request

## License

This project is licensed under the MIT License - see the LICENSE file for details.

## Acknowledgments

- The BitTorrent community for creating and maintaining the DHT protocol
- The C++ community for developing the language and libraries that make this project possible
