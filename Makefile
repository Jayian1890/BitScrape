# Top-level Makefile for BitScrape

TOP := $(CURDIR)
BUILD_DIR ?= build
LIB_DIR := $(BUILD_DIR)/lib
BIN_DIR := $(BUILD_DIR)/bin
CXX ?= g++
CXXFLAGS ?= -std=c++23 -Wall -Wextra -Wpedantic
ifeq ($(DEBUG),1)
CXXFLAGS += -g -O0
else
CXXFLAGS += -O2
endif
PREFIX ?= /usr/local

# Discover modules by the presence of CMakeLists.txt (keep parity with original layout)
MODULE_DIRS := $(shell for d in modules/*; do if [ -d $$d ] && [ -f $$d/CMakeLists.txt ]; then basename $$d; fi; done)
MODULES := $(MODULE_DIRS)

.PHONY: all modules apps clean install test help

all: modules apps

modules: $(MODULES:%=lib/%)

lib/%:
	$(MAKE) -C modules/$* TOP=$(TOP) BUILD_DIR=$(TOP)/$(BUILD_DIR) LIB_DIR=$(TOP)/$(LIB_DIR) CXX="$(CXX)" CXXFLAGS="$(CXXFLAGS)"

apps:
	$(MAKE) -C apps/cli TOP=$(TOP) BUILD_DIR=$(TOP)/$(BUILD_DIR) LIB_DIR=$(TOP)/$(LIB_DIR) BIN_DIR=$(TOP)/$(BIN_DIR) CXX="$(CXX)" CXXFLAGS="$(CXXFLAGS)" MODULES="$(MODULES)"

clean:
	rm -rf $(BUILD_DIR)/* || true

install: all
	install -d $(DESTDIR)$(PREFIX)/bin
	install -m 0755 $(BIN_DIR)/bitscrape_cli $(DESTDIR)$(PREFIX)/bin/

test:
	@echo "Running module tests..."
	@set -e; for m in $(MODULES); do echo "=== Testing $$m ==="; $(MAKE) -C modules/$$m test TOP=$(TOP) BUILD_DIR=$(BUILD_DIR) LIB_DIR=$(LIB_DIR) CXX="$(CXX)" CXXFLAGS="$(CXXFLAGS)"; done || true

help:
	@echo "Targets:"
	@echo "  all      - build libraries and apps"
	@echo "  modules  - build all module libraries"
	@echo "  apps     - build applications (apps/cli)"
	@echo "  clean    - clean build artifacts"
	@echo "  install  - install binaries to $(PREFIX)"
	@echo "Variables:"
	@echo "  CXX      - compiler (default g++)"
	@echo "  CXXFLAGS - compiler flags (default -std=c++23 -Wall ... )"
	@echo "  BUILD_DIR- build directory (default build)"
