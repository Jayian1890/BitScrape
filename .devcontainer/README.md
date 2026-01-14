# BitScrape Development Container

This directory contains the development container configuration for the BitScrape project, providing a consistent C++23 development environment.

## Features

- **C++23 Support**: GCC 11+ and Clang 14+ with full C++23 standard support
- **Complete Build Environment**: All necessary tools for building and testing BitScrape
- **Development Tools**: GDB debugger, Valgrind, coverage analysis with gcovr
- **VS Code Integration**: Pre-configured extensions and settings for optimal C++ development
- **Persistent Build Cache**: Build artifacts persist between container restarts
- **Network Access**: Support for DHT bootstrap nodes and web interface

## Prerequisites

- [Visual Studio Code](https://code.visualstudio.com/)
- [Dev Containers extension](https://marketplace.visualstudio.com/items?itemName=ms-vscode-remote.remote-containers)
- [Docker](https://www.docker.com/get-started)

## Getting Started

### Option 1: VS Code Dev Containers (Recommended)

1. Open the BitScrape project in VS Code
2. When prompted, click "Reopen in Container" or use Command Palette: `Dev Containers: Reopen in Container`
3. The container will build automatically and open the project

### Option 2: Docker Compose

If you prefer using Docker Compose directly:

```bash
# Start the development container
docker-compose -f .devcontainer/docker-compose.yml up -d

# Attach to the running container
docker-compose -f .devcontainer/docker-compose.yml exec bitscrape-dev bash
```

### Option 3: Manual Docker Build

```bash
# Build the container
docker build -f .devcontainer/Dockerfile -t bitscrape-dev .

# Run the container
docker run -it --rm \
  -v $(pwd):/workspace \
  -v $(pwd)/build:/workspace/build \
  -v $(pwd)/data:/workspace/data \
  --cap-add=SYS_PTRACE \
  --security-opt seccomp=unconfined \
  -p 8080:8080 \
  bitscrape-dev bash
```

## Development Workflow

Once inside the container:

```bash
# Build the entire project
make

# Build with debug flags
make DEBUG=1

# Build a specific module
make -C modules/core

# Run all tests
make test

# Run tests for a specific module
make -C modules/core test

# Generate coverage report
make coverage

# Install the project
make install PREFIX=/usr/local
```

## Debugging

The container is configured for debugging with:

- **GDB**: `gdb ./build/bin/bitscrape_cli`
- **LLDB**: `lldb ./build/bin/bitscrape_cli`
- **Valgrind**: `valgrind --tool=memcheck ./build/bin/bitscrape_cli`

## Web Interface

The web interface runs on port 8080 and will be automatically forwarded when using VS Code Dev Containers.

```bash
# Start the web interface
./build/bin/bitscrape_cli --web-port=8080
```

## VS Code Extensions

The following extensions are automatically installed and configured:

- **C/C++ Extension Pack**: Complete C++ development support
- **Makefile Tools**: Enhanced Makefile support
- **CMake Tools**: CMake integration (if needed)
- **LLDB Debugger**: Native debugger support
- **Python**: For build scripts and coverage tools

## Customization

### Adding Dependencies

To add system dependencies, modify `.devcontainer/Dockerfile`:

```dockerfile
RUN apt-get update && apt-get install -y \
    your-package-here \
    && rm -rf /var/lib/apt/lists/*
```

### VS Code Settings

Modify `.devcontainer/devcontainer.json` to change VS Code settings and add more extensions.

### Build Configuration

The container uses the project's Makefile build system. Modify `Makefile` for custom build configurations.

## Troubleshooting

### Build Issues

If you encounter build issues:

1. Clean the build directory: `make clean`
2. Rebuild from scratch: `make`
3. Check compiler version: `g++ --version`

### Permission Issues

The container runs as a non-root user (`vscode`). If you need root access:

```bash
sudo apt-get update
```

### Network Issues

For DHT functionality, ensure the container can reach the internet:

```bash
ping router.bittorrent.com
```

### Performance Issues

If builds are slow, try:
- Increasing Docker memory allocation
- Using build cache mounts
- Running parallel builds: `make -j$(nproc)`

## Project Structure

The development container preserves the original project structure while providing isolated build and data directories:

```
/workspace/
├── include/          # Public headers
├── modules/          # Module implementations
├── apps/            # Applications (CLI, GUI, Web)
├── tests/           # Unit tests
├── build/           # Build artifacts (mounted)
├── data/            # Runtime data (mounted)
└── .devcontainer/   # Dev container configuration
```
