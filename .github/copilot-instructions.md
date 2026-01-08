# Copilot / AI agent instructions for BitScrape

Goal: make an AI coding agent productive quickly by documenting the project shape, common workflows, and repository conventions.

## Quick start (build & run)
- Build everything: `make` (top-level Makefile). For debug builds: `make DEBUG=1`.
- Build CLI only: `make -C apps/cli` (or `make -C apps/cli DEBUG=1`). The CLI binary is `build/bin/bitscrape_cli`.
- Run unit tests: `make test` (runs tests for all modules). Single-module tests: `make -C modules/<module> test`.
- Coverage: install `gcovr` (`pip install gcovr`) then `make coverage` → produces `lcov.info` in repo root.

## Big picture (short)
- Modular, event-driven architecture: modules/<module> are static libraries (archives in `build/lib/lib<module>.a`), apps (CLI/Web) link those libs.
- Core pieces: `modules/core` (Controller, Configuration), `modules/dht`, `modules/bittorrent`, `modules/storage` (SQLite wrapper + StorageManager), `modules/web` (HTTP server + API handlers), `modules/event` (event bus system).
- App examples: `apps/cli` is the canonical entrypoint; `apps/cli/src/main.cpp` shows CLI flags, how Configuration is used, and how the Web UI is started.

## Project conventions & patterns an agent should follow
- C++23 is required; compile flags are set in Makefiles (`-std=c++23 -Wall -Wextra -Wpedantic`). See top-level `Makefile`.
- Module layout: `modules/<name>/include/bitscrape/...` and `modules/<name>/src/...`. To add a module, add a `Makefile` that includes `modules/module.mk`.
- Static libs: modules build to `build/lib/lib<module>.a`. Apps link with `MODULES := $(MODULES)` to locate the libs.
- Tests: vendored doctest header under `third_party` and `tests/doctest_main.cpp` provide the test runner. Tests live under `modules/<module>/tests/unit` and compile to `build/tests/<module>/run_tests`.
- Test helper: include `#include <bitscrape/testing.hpp>` for a GoogleTest-like API mapped to doctest macros.
- Configuration: `bitscrape.json` is the default config file (template created under `build/bitscrape.json.template`). Use `bitscrape::core::Configuration` to read and write keys (e.g., `database.path`, `web.auto_start`, `web.static_dir`). See `modules/core/src/configuration.cpp`.
- Storage: uses an internal Database wrapper (SQLite-like) under `modules/storage`. Default DB path: `data/default.db` or `database.path` in configuration.
- Logging/beacon: use `modules/beacon` (`bitscrape::beacon::Beacon`) for structured messages (see `include/bitscrape/types/beacon_types.hpp`).
- Event-driven: components communicate via events (see `modules/event`). The `Controller` wires modules together (see `modules/core/src/controller.cpp`).

## How to debug / common developer workflows
- Quick local debug: build with debug flags: `make DEBUG=1` then run `./build/bin/bitscrape_cli --help` or run the binary directly.
- Run a single test binary: `./build/tests/<module>/run_tests` (doctest supports test filtering and runner flags).
- Change compiler: override `CXX` and `CXXFLAGS` at make time: `make CXX=clang++ CXXFLAGS="-std=c++23 -O2 -Wall"`.
- CI/coverage: `make coverage` rebuilds with coverage flags and writes `lcov.info` in the repo root.

### VS Code — tasks & launch (macOS)
- Recommended extensions: **C/C++ (ms-vscode.cpptools)** and **Makefile Tools** (optional).
- Create workspace `.vscode/tasks.json` to run builds and tests, and `.vscode/launch.json` to debug the compiled binary.

Sample `tasks.json` (add under `.vscode/`):

```json
.vscode/c_cpp_properties.json .vscode/launch.json .vscode/settings.json .vscode/tasks.json
```

Sample `launch.json` (add under `.vscode/`):

```json
{
    "version": "2.0.0",
    "tasks": [
        {
            "label": "make all",
            "type": "shell",
            "command": "make",
            "args": ["DEBUG=1", "all"],
            "options": {
                "cwd": "${workspaceFolder}"
            },
            "group": "build",
            "problemMatcher": "$msvc"
        },
        {
            "label": "make app",
            "type": "shell",
            "command": "make",
            "args": ["DEBUG=1", "RUN_TESTS=0", "apps"],
            "options": {
                "cwd": "${workspaceFolder}"
            },
            "group": "build",
            "problemMatcher": "$msvc"
        },
        {
            "label": "run bitscrape_cli",
            "type": "shell",
            "command": "${workspaceFolder}/build/bin/bitscrape_cli",
            "options": {
                "cwd": "${workspaceFolder}"
            },
            "problemMatcher": "$msvc"
        },
        {
            "label": "run tests",
            "type": "shell",
            "command": "make",
            "args": ["test"],
            "options": {
                "cwd": "${workspaceFolder}"
            },
            "group": "test",
            "problemMatcher": "$msvc"
        }
    ]
}
```

- Tips:
  - Use the `preLaunchTask` to ensure the binary is built with `DEBUG=1`.
  - To debug unit-tests, set `program` to `build/tests/<module>/run_tests` and pass doctest runner flags as `args` (e.g., `--success --repeat 1`).
  - On macOS prefer `MIMode: "lldb"`; on Linux use `gdb`.


## Files an agent should read to understand code paths quickly
- `README.md` (architecture & usage), `BUILD.md` (detailed build/test/coverage instructions), `Makefile` and `modules/module.mk` (build patterns).
- `apps/cli/src/main.cpp` (CLI options, configuration usage, web interface bootstrap).
- `modules/core/*` (especially `controller.hpp/cpp` and `configuration.hpp/cpp`).
- `modules/storage/*` (database wrapper, `StorageManager`, `QueryInterface`).
- `tests/doctest_main.cpp` and `include/bitscrape/testing.hpp` (test runner and test API conventions).

## Small actionable rules for code changes and PRs
- Keep changes module-scoped where possible. Add code in `modules/<module>` and export targets via the module Makefile.
- Add unit tests under `modules/<module>/tests/unit` using the `TEST_CASE` macro (or `TEST`/`ASSERT_*` via `bitscrape/testing.hpp`).
- Prefer the existing sync/async helpers in `Configuration`/`Database` rather than adding blocking calls.
- Avoid introducing additional test mains (there is a single `tests/doctest_main.cpp` used by modules).
- When adding public headers, place them under `modules/<module>/include/bitscrape/<module>/` so the centralized include scanning picks them up.

## Integration points & external dependencies
- No heavy external C++ deps; doctest is vendored. The build assumes only a C++23-capable compiler and `make`.
- Runtime interacts with network endpoints (DHT bootstrap nodes, trackers) and writes to a local SQLite-backed database (see `database.path` in config).

---
If you'd like I can: (1) merge this into an existing `.github/copilot-instructions.md` if one exists, (2) add short examples for common PR types (fix, feature, test), or (3) expand sections with copyable shell snippets. Which should I do next?