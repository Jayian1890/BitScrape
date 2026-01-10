# Top-level Makefile for BitScrape

# Auto-detect number of CPU cores for parallel builds
NPROCS := 1
ifeq ($(OS),Windows_NT)
    NPROCS := $(shell echo %NUMBER_OF_PROCESSORS%)
else
    UNAME_S := $(shell uname -s)
    ifeq ($(UNAME_S),Linux)
        NPROCS := $(shell nproc)
    endif
    ifeq ($(UNAME_S),Darwin)
        NPROCS := $(shell sysctl -n hw.ncpu)
    endif
endif
MAKEFLAGS += -j$(shell echo $$(($(NPROCS)*2)))

TOP := $(CURDIR)
BUILD_DIR ?= build
LIB_DIR := $(BUILD_DIR)/lib
BIN_DIR := $(BUILD_DIR)/bin
CXX ?= g++
CXXFLAGS ?= -std=c++23 -Wall -Wextra -Wpedantic
LDFLAGS ?=
COVERAGE ?= 0
ifeq ($(DEBUG),1)
CXXFLAGS += -g -O0
else
CXXFLAGS += -O2
endif
ifeq ($(COVERAGE),1)
CXXFLAGS += --coverage -O0
LDFLAGS += --coverage
endif
PREFIX ?= /usr/local

# Discover modules by the presence of a module Makefile (Make-only flow)
MODULE_DIRS := $(shell for d in modules/*; do if [ -d $$d ] && [ -f $$d/Makefile ]; then basename $$d; fi; done)
MODULES := $(MODULE_DIRS)

.PHONY: all modules apps clean install test coverage apps-coverage help compile_commands

all: modules apps

modules: $(MODULES:%=lib/%)

lib/%:
	$(MAKE) -C modules/$* TOP=$(TOP) BUILD_DIR=$(TOP)/$(BUILD_DIR) LIB_DIR=$(TOP)/$(LIB_DIR) CXX="$(CXX)" CXXFLAGS="$(CXXFLAGS)" LDFLAGS="$(LDFLAGS)" COVERAGE=$(COVERAGE) RUN_TESTS=0

apps: modules
	$(MAKE) -C apps/cli TOP=$(TOP) BUILD_DIR=$(TOP)/$(BUILD_DIR) LIB_DIR=$(TOP)/$(LIB_DIR) BIN_DIR=$(TOP)/$(BIN_DIR) CXX="$(CXX)" CXXFLAGS="$(CXXFLAGS)" LDFLAGS="$(LDFLAGS)" MODULES="$(MODULES)" COVERAGE=$(COVERAGE)

clean:
	rm -rf $(BUILD_DIR)/* || true

install: all
	install -d $(DESTDIR)$(PREFIX)/bin
	install -m 0755 $(BIN_DIR)/bitscrape_cli $(DESTDIR)$(PREFIX)/bin/

test: modules
	@echo "Running module tests..."
	@set -e; for m in $(MODULES); do echo "=== Testing $$m ==="; $(MAKE) -C modules/$$m test TOP=$(TOP) BUILD_DIR=$(TOP)/$(BUILD_DIR) LIB_DIR=$(TOP)/$(LIB_DIR) CXX="$(CXX)" CXXFLAGS="$(CXXFLAGS)" LDFLAGS="$(LDFLAGS)" COVERAGE=$(COVERAGE); done || true

coverage:
	@echo "Rebuilding with coverage instrumentation and running tests..."
	@echo "Cleaning previous coverage artifacts..."
	@find $(TOP) -name '*.gcda' -o -name '*.gcno' -exec rm -f {} + || true
	@rm -f $(TOP)/lcov.info || true
	@rm -rf $(TOP)/coverage || true
	$(MAKE) COVERAGE=1 RUN_TESTS=1 test
	@command -v gcovr >/dev/null 2>&1 || { echo "gcovr is required to export coverage (install with 'pip install gcovr')."; exit 1; }
	# Map source paths to the modules/core area to ensure GCOVR can locate sources for coverage.
	# Root is set to modules/core so that paths like tests/unit/... map to modules/core/tests/unit/...
	@gcovr --root $(TOP)/modules/core --filter $(TOP)/modules/core --filter $(TOP)/modules --filter $(TOP)/apps --exclude $(TOP)/third_party --lcov --output $(TOP)/lcov.info
	@echo "Coverage report generated at $(TOP)/lcov.info"

apps-coverage: apps coverage
	@echo "To view the coverage report, use a tool like 'genhtml' or upload 'lcov.info' to a coverage service."

compile_commands:
	@echo "Generating compile_commands.json..."
	@python3 scripts/generate_compile_commands.py --build-dir $(BUILD_DIR) --cxx "$(CXX)" --cxxflags "$(CXXFLAGS)"
	@echo "Wrote $(BUILD_DIR)/compile_commands.json"

help:
	@echo "Targets:"
	@echo "  all      - build libraries and apps"
	@echo "  modules  - build all module libraries"
	@echo "  apps     - build applications (apps/cli)"
	@echo "  coverage - rebuild with coverage, run tests, export lcov.info"
	@echo "  clean    - clean build artifacts"
	@echo "  install  - install binaries to $(PREFIX)"
	@echo "Variables:"
	@echo "  CXX      - compiler (default g++)"
	@echo "  CXXFLAGS - compiler flags (default -std=c++23 -Wall ... )"
	@echo "  BUILD_DIR- build directory (default build)"
