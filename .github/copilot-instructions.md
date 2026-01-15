# Copilot / AI agent instructions for BitScrape

Goal: make an AI coding agent productive quickly by documenting the project shape, common workflows, and repository conventions.

## Quick start (build & run)
- **Build System**: CMake (replace any references to `make` with `cmake`).
- **Configure**: `cmake -B build -S . -DCMAKE_BUILD_TYPE=Debug` (or `Release`).
- **Build**: `cmake --build build` (parallel build: `cmake --build build -j$(nproc)`).
- **Run CLI**: `./build/bin/bitscrape_cli --help`.
- **Run Tests**: `ctest --test-dir build --output-on-failure` (run all tests with detailed output on failure).
- **Clean Build**: `rm -rf build && cmake -B build -S . -DCMAKE_BUILD_TYPE=Debug` (start fresh if needed).

## Big picture (short)
- **Architecture**: Modular, event-driven. Modules are static libraries, apps link them.
- **Directory Structure**:
  - `include/bitscrape/<module>/`: Public headers.
  - `modules/<module>/src/`: Module implementation files.
  - `apps/`: Applications and UIs (e.g. `apps/cli`, `apps/web`).
  - `CMakeLists.txt`: Root and per-module build definitions.
- **Key Modules**:
  - `modules/core`: Controller (orchestration), Configuration.
  - `modules/dht`, `modules/bittorrent`: Protocol implementations.
  - `modules/storage`: SQLite database wrapper (`StorageManager`).
  - `modules/event`: Event bus system.
  - `modules/beacon`: Logging system.
- **Applications**:
  - `apps/cli`: Command-line interface.
  - `apps/web`: Web UI (HTTP server, API handlers, WebSocket).

## Project conventions & patterns an agent should follow
- **Language**: C++23 is required.
- **Build System**: **CMake** is the source of truth. Ensure `CMakeLists.txt` files are updated when adding files.
- **Headers**: Always place public headers in `include/bitscrape/<module>/`.
- **Implementation**: Use the Pimpl idiom (`class Impl`) in `.cpp` files to hide private details (e.g., `Controller::Impl` in `modules/core/src/controller.cpp`).
- **Naming**:
  - **Namespaces**: Nested `bitscrape::<module>` (e.g., `bitscrape::core`).
  - **Classes**: `PascalCase` (e.g., `StorageManager`).
  - **Methods/Functions**: `snake_case` (e.g., `start_async`, `get_string`).
  - **Variables**: `snake_case` (members often end with `_`, e.g., `config_`, `is_running_`).
- **Logging**: Use `bitscrape::beacon::Beacon`. Typically `beacon_->info("Message", types::BeaconCategory::GENERAL)`.
- **Async**: Heavy use of `std::future`, `std::async` for non-blocking operations.
- **Dependencies**: Codebase is self-contained (vendored `doctest` if used), minimal external deps.
- **Git Commit Messages**: Follow the Conventional Commits specification:
  - Format: `<type>(<scope>): <subject>`
  - Types:
    - `feat`: New feature
    - `fix`: Bug fix
    - `docs`: Documentation only changes
    - `style`: Changes that do not affect the meaning of the code (white-space, formatting, etc)
    - `refactor`: A code change that neither fixes a bug nor adds a feature
    - `perf`: A code change that improves performance
    - `test`: Adding missing tests or correcting existing tests
    - `build`: Changes that affect the build system or external dependencies
    - `ci`: Changes to our CI configuration files and scripts
    - `chore`: Other changes that don't modify src or test files
    - `revert`: Reverts a previous commit
- **Git Workflow**:
  - **Atomic Task Commits**: AI MUST commit and push changes immediately after completing each distinct user task or logical unit of work.
  - **No Batching**: Do NOT combine changes from separate tasks into a single commit. Process each request's changes independently to maintain a clean and revertible history.

## Files an agent should read to understand code paths quickly
- **Root Build**: `CMakeLists.txt` (Project settings, compiler flags).
- **App Entry**: `apps/cli/src/main.cpp` (CLI setup, wiring).
- **Orchestration**: `modules/core/src/controller.cpp` (The brain of the app, wiring modules).
- **Configuration**: `include/bitscrape/core/configuration.hpp`.
- **Storage**: `modules/storage/src/storage_manager.cpp`.

## Debugging & Workflows
- **Debug Build**: Ensure `-DCMAKE_BUILD_TYPE=Debug` is used.
- **Run**: Execute `./build/bin/bitscrape_cli`.
- **VS Code**: Use CMake Tools extension. `Ctrl+Shift+P` -> `CMake: Configure` -> `CMake: Build`.

## Integration points
- **Database**: SQLite DB file defaults to `data/bitscrape.db` (created by `modules/storage`).
- **Network**: Binds to ports (default 6881) for DHT/BitTorrent.

## Testing
- **Test Framework**: Uses CTest with CMake.
- **Running Tests**: After building, run `ctest --test-dir build --output-on-failure`.
- **Test Location**: Test files should be organized alongside module code or in dedicated test directories.
- **Writing Tests**: Follow existing test patterns in the repository. Tests should be focused, fast, and independent.

## Security best practices
- **Never commit secrets**: API keys, passwords, or sensitive data should never be in source code.
- **Input validation**: Always validate and sanitize user input, especially from network sources.
- **Error handling**: Handle errors gracefully without exposing internal details.
- **Dependencies**: Minimize external dependencies; when adding new dependencies, ensure they are from trusted sources.

## Common pitfalls to avoid
- **Don't modify generated files**: Files in `build/` are generated; changes should be made to source files.
- **Thread safety**: Be careful with shared state; use proper locking mechanisms from `modules/lock` (e.g., `LockManager`, `LockGuard` for resource-based locking).
- **Memory management**: Use smart pointers (`std::unique_ptr`, `std::shared_ptr`) over raw pointers.
- **Async operations**: Always handle futures and async operations properly; don't let them dangle.
- **Platform differences**: Test cross-platform changes on multiple OSes (Linux, macOS, Windows).

## Code quality guidelines
- **Keep functions small**: Each function should do one thing well.
- **Avoid deep nesting**: Refactor deeply nested code into separate functions.
- **Use RAII**: Resource Acquisition Is Initialization for all resource management.
- **Const correctness**: Use `const` wherever applicable for parameters and methods.
- **Error messages**: Provide clear, actionable error messages through the Beacon logging system.

## When adding new features
1. **Update CMakeLists.txt**: Add new source files to the appropriate module's `CMakeLists.txt`.
2. **Public headers**: Place in `include/bitscrape/<module>/`, implementation in `modules/<module>/src/`.
3. **Follow Pimpl**: Use the Pimpl idiom for implementation details.
4. **Add tests**: Write tests for new functionality.
5. **Update documentation**: Keep README.md and relevant docs in sync with code changes.
6. **Event integration**: If the feature needs to communicate with other modules, use the event system.