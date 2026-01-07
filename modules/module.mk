# Shared module make fragment. Intended to be included by modules/*/Makefile

TOP ?= $(abspath $(TOP))
BUILD_DIR ?= build
LIB_DIR ?= $(BUILD_DIR)/lib
CXX ?= g++
CXXFLAGS ?= -std=c++23 -Wall -Wextra -Wpedantic

SRCDIR := src
OBJDIR := $(BUILD_DIR)/modules/$(MODULE)
SRCS := $(shell find $(SRCDIR) -name '*.cpp')
OBJS := $(patsubst $(SRCDIR)/%.cpp,$(OBJDIR)/%.o,$(SRCS))
INCLUDES := -Iinclude $(shell for d in $(TOP)/modules/*/include; do if [ -d $$d ]; then printf " -I%s" $$d; fi; done)

.PHONY: all clean test

all: $(LIB_DIR)/lib$(MODULE).a

$(OBJDIR)/%.o: $(SRCDIR)/%.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

$(LIB_DIR)/lib$(MODULE).a: $(OBJS)
	@mkdir -p $(dir $@)
	ar rcs $@ $^

# Unit test support (optional)
# Configure GTEST_INCLUDES and GTEST_LIBS if tests require non-system locations
GTEST_INCLUDES ?= -I/usr/local/include
GTEST_LIBS ?= -lgtest -lgtest_main -pthread
TEST_SRCDIR := tests/unit
TEST_BIN_DIR := $(BUILD_DIR)/tests/$(MODULE)
TEST_SRCS := $(shell if [ -d $(TEST_SRCDIR) ]; then find $(TEST_SRCDIR) -name '*.cpp'; fi)
TEST_OBJS := $(patsubst $(TEST_SRCDIR)/%.cpp,$(TEST_BIN_DIR)/%.o,$(TEST_SRCS))

test: $(TEST_SRCS)
	@if [ -z "$(TEST_SRCS)" ]; then echo "No unit tests for $(MODULE)"; exit 0; fi
	$(MAKE) -f $(lastword $(MAKEFILE_LIST)) $(TEST_BIN_DIR)/run_tests BUILD_DIR=$(BUILD_DIR) LIB_DIR=$(LIB_DIR)

$(TEST_BIN_DIR)/%.o: $(TEST_SRCDIR)/%.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) $(INCLUDES) $(GTEST_INCLUDES) -c $< -o $@

$(TEST_BIN_DIR)/run_tests: $(TEST_OBJS) $(LIB_DIR)/lib$(MODULE).a
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -o $@ $(TEST_OBJS) $(LIB_DIR)/lib$(MODULE).a $(GTEST_LIBS)

clean:
	rm -rf $(OBJDIR) $(LIB_DIR)/lib$(MODULE).a $(TEST_BIN_DIR)
