# Shared module make fragment. Intended to be included by modules/*/Makefile

TOP ?= $(abspath $(CURDIR)/../..)
BUILD_DIR ?= $(TOP)/build
LIB_DIR ?= $(BUILD_DIR)/lib
CXX ?= g++
CXXFLAGS ?= -std=c++23 -Wall -Wextra -Wpedantic
DOCTEST_DIR ?= $(TOP)/third_party
RUN_TESTS ?= 1

SRCDIR := src
OBJDIR := $(BUILD_DIR)/modules/$(MODULE)
SRCS := $(shell find $(SRCDIR) -name '*.cpp')
OBJS := $(patsubst $(SRCDIR)/%.cpp,$(OBJDIR)/%.o,$(SRCS))
INCLUDES := -Iinclude -I$(TOP)/include $(shell for d in $(TOP)/modules/*/include; do if [ -d $$d ]; then printf " -I%s" $$d; fi; done)
TEST_INCLUDES := $(INCLUDES) -I$(DOCTEST_DIR)

.PHONY: all clean test maybe_test

all: clean maybe_test $(LIB_DIR)/lib$(MODULE).a

$(OBJDIR)/%.o: $(SRCDIR)/%.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

$(LIB_DIR)/lib$(MODULE).a: $(OBJS)
	@mkdir -p $(dir $@)
	ar rcs $@ $^

# Unit test support (optional)
TEST_SRCDIR := tests/unit
TEST_BIN_DIR := $(BUILD_DIR)/tests/$(MODULE)
TEST_SRCS := $(shell if [ -d $(TEST_SRCDIR) ]; then find $(TEST_SRCDIR) -name '*.cpp'; fi)
TEST_OBJS := $(patsubst $(TEST_SRCDIR)/%.cpp,$(TEST_BIN_DIR)/%.o,$(TEST_SRCS))
TEST_MAIN_OBJ := $(BUILD_DIR)/tests/doctest_main.o
# Default to linking only the module under test; additional libs can override TEST_LIBS if needed.
TEST_LIBS ?= $(LIB_DIR)/lib$(MODULE).a

test: $(TEST_SRCS)
	@if [ -z "$(TEST_SRCS)" ]; then \
		echo "No unit tests for $(MODULE)"; \
	else \
		$(MAKE) $(TEST_BIN_DIR)/run_tests; \
	fi

ifeq ($(RUN_TESTS),1)
maybe_test: test
else
maybe_test:
	@echo "Skipping tests for $(MODULE) (RUN_TESTS=$(RUN_TESTS))"
endif

$(TEST_BIN_DIR)/%.o: $(TEST_SRCDIR)/%.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) $(TEST_INCLUDES) -D__FILE__='"modules/$(MODULE)/tests/unit/$(notdir $<)"' -c $< -o $@


$(TEST_MAIN_OBJ): $(TOP)/tests/doctest_main.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) $(TEST_INCLUDES) -c $< -o $@

$(TEST_BIN_DIR)/run_tests: $(TEST_OBJS) $(TEST_MAIN_OBJ) $(TEST_LIBS)
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -o $@ $(TEST_OBJS) $(TEST_MAIN_OBJ) $(TEST_LIBS)

clean:
	rm -rf $(OBJDIR) $(LIB_DIR)/lib$(MODULE).a $(TEST_BIN_DIR) $(TEST_MAIN_OBJ)
