# BitScrape Development Container

This directory contains the development container configuration for the BitScrape project, providing a consistent C++23 development environment.

## Features

- **C++23 Support**: Proprely configured GCC 12 and Clang 15 with full C++23 standard support.
- **Complete Build Environment**: Based on Microsoft's optimized C++ development image.
- **Development Tools**: GDB, LLDB, Valgrind, and coverage analysis with `gcovr`.
- **VS Code Integration**: Pre-configured extensions (CMake, C++, GitLens) and settings.
- **Docker Compose Orchestration**: Uses Docker Compose for better volume management and network isolation.
- **Persistent Build Cache**: Named volumes ensure build artifacts persist and don't conflict with your host OS.

## Prerequisites

- [Visual Studio Code](https://code.visualstudio.com/)
- [Dev Containers extension](https://marketplace.visualstudio.com/items?itemName=ms-vscode-remote.remote-containers)
- [Docker](https://www.docker.com/get-started)

## Getting Started

### Option 1: VS Code Dev Containers (Recommended)

1. Open the BitScrape project in VS Code.
2. When prompted, click **"Reopen in Container"** or use the Command Palette (`Cmd+Shift+P` on Mac) and select `Dev Containers: Reopen in Container`.
3. The container will build automatically and initialize the CMake build system.

### Option 2: Docker Compose

If you prefer using Docker Compose directly:

```bash
# Start the development container
docker-compose -f .devcontainer/docker-compose.yml up -d

# Attach to the running container
docker-compose -f .devcontainer/docker-compose.yml exec bitscrape-dev bash
```

## Development Workflow

The environment is optimized for **CMake**. Once inside the container:

```bash
# The project is already configured via postCreateCommand
# To build everything:
cmake --build build/container -j$(nproc)

# To run tests:
ctest --test-dir build/container

# To run the CLI:
./build/container/apps/cli/bitscrape-cli --help
```

## Debugging

The container is fully configured for debugging. You can use the VS Code "Run and Debug" side bar to start debugging with the pre-configured LLDB or GDB setups.

- **Valgrind**: `valgrind --tool=memcheck ./build/container/apps/cli/bitscrape-cli`
- **Coverage**: `cmake -DENABLE_COVERAGE=ON ..` (if supported by CMakeLists)

## Web Interface

The web interface runs on port 8080 and is automatically forwarded by VS Code.

## Customization

### Adding Dependencies

To add system dependencies, modify `.devcontainer/Dockerfile` and rebuild the container.

### VS Code Settings

Modify `.devcontainer/devcontainer.json` to change default editor behavior or add extensions.

## Troubleshooting

### Rebuilding the Container

If you change the Dockerfile, you must rebuild the container:
`Dev Containers: Rebuild Container` in the Command Palette.

### Architecture Conflicts

This setup uses a named volume for the `build` directory (`build/container`) to prevent conflicts between your host OS and the Linux container. Always build inside `/workspace/build/container` when in the devcontainer.
