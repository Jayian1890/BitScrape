# BitScrape

BitScrape is a BitTorrent DHT crawler and metadata scraper designed to efficiently discover and collect metadata from the BitTorrent distributed hash table network.

## Features

- DHT protocol implementation (Mainline, Kademlia, Azureus variants)
- Node discovery and routing
- Infohash discovery through get_peers queries
- BitTorrent metadata exchange protocol
- Persistent storage of discovered data
- Web-based monitoring interface
- CLI and GUI applications
- Cross-platform support (macOS, Linux, Windows)

## Project Structure

```
bitscrape/
├── modules/           # Core modules, each as an independent library
├── apps/              # Application executables (CLI, GUI)
├── tools/             # Development and utility tools
├── docs/              # Documentation
├── resources/         # Resources (icons, web assets, etc.)
├── tests/             # Integration and system tests
├── examples/          # Example code showing how to use modules
├── scripts/           # Build and utility scripts
└── third_party/       # Third-party dependencies (if any)
```

## Building

### Prerequisites

- C++23 compatible compiler (GCC 11+, Clang 14+, or MSVC 19.29+)
- Meson build system
- Ninja build tool
- Python 3.6+
- fmt library
- libcurl
- pkg-config (for dependency resolution)

### Build Commands

```bash
# Configure and build
./build.sh

# Build with specific options
./build.sh --release      # Release build
./build.sh --debug --asan # Debug build with AddressSanitizer
./build.sh --no-tests     # Skip building tests
./build.sh --clean        # Clean build

# Or use Meson directly
meson setup build
meson compile -C build
```

## Usage

### CLI Application

```bash
# Start DHT crawler
./build/apps/cli/bitscrape_cli --crawl

# Download metadata for specific infohash
./build/apps/cli/bitscrape_cli --metadata <infohash>

# Start web interface
./build/apps/cli/bitscrape_cli --web --port 8080
```

### GUI Application

Launch the GUI application from your desktop environment or run:

```bash
./build/apps/gui/bitscrape_gui
```

## Documentation

See the [docs](docs/) directory for detailed documentation.

## License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.