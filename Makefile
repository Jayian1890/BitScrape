# Top-level Makefile for BitScrape

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

.PHONY: all modules apps clean install test coverage apps-coverage help

all: modules apps

modules: $(MODULES:%=lib/%)

lib/%:
	$(MAKE) -C modules/$* TOP=$(TOP) BUILD_DIR=$(TOP)/$(BUILD_DIR) LIB_DIR=$(TOP)/$(LIB_DIR) CXX="$(CXX)" CXXFLAGS="$(CXXFLAGS)" LDFLAGS="$(LDFLAGS)" COVERAGE=$(COVERAGE)

apps: modules
	$(MAKE) -C apps/cli TOP=$(TOP) BUILD_DIR=$(TOP)/$(BUILD_DIR) LIB_DIR=$(TOP)/$(LIB_DIR) BIN_DIR=$(TOP)/$(BIN_DIR) CXX="$(CXX)" CXXFLAGS="$(CXXFLAGS)" LDFLAGS="$(LDFLAGS)" MODULES="$(MODULES)" COVERAGE=$(COVERAGE)

clean:
	rm -rf $(BUILD_DIR)/* || true

install: all
	install -d $(DESTDIR)$(PREFIX)/bin
	install -m 0755 $(BIN_DIR)/bitscrape_cli $(DESTDIR)$(PREFIX)/bin/

test:
	@echo "Running module tests..."
	@set -e; for m in $(MODULES); do echo "=== Testing $$m ==="; $(MAKE) -C modules/$$m test TOP=$(TOP) BUILD_DIR=$(BUILD_DIR) LIB_DIR=$(LIB_DIR) CXX="$(CXX)" CXXFLAGS="$(CXXFLAGS)" LDFLAGS="$(LDFLAGS)" COVERAGE=$(COVERAGE); done || true

coverage:
	@echo "Rebuilding with coverage instrumentation and running tests..."
	$(MAKE) clean
	$(MAKE) COVERAGE=1 RUN_TESTS=1 test
	@command -v gcovr >/dev/null 2>&1 || { echo "gcovr is required to export coverage (install with 'pip install gcovr')."; exit 1; }
	@gcovr --root $(TOP) --filter $(TOP)/modules --filter $(TOP)/apps --exclude $(TOP)/third_party --lcov --output $(TOP)/lcov.info
	@echo "Coverage report generated at $(TOP)/lcov.info"

apps-coverage: apps coverage
	@echo "To view the coverage report, use a tool like 'genhtml' or upload 'lcov.info' to a coverage service."

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
