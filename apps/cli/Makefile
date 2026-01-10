TOP ?= $(abspath $(TOP))
BUILD_DIR ?= build
BIN_DIR ?= $(BUILD_DIR)/bin
LIB_DIR ?= $(BUILD_DIR)/lib
CXX ?= g++
CXXFLAGS ?= -std=c++23 -Wall -Wextra -Wpedantic
LDFLAGS ?=
COVERAGE ?= 0
ifeq ($(COVERAGE),1)
CXXFLAGS += --coverage -O0
LDFLAGS += --coverage
endif
MODULES ?= $(MODULES)

APP := bitscrape_cli
SRCDIR := src
OBJDIR := $(BUILD_DIR)/apps/cli
SRCS := $(shell find $(SRCDIR) -name '*.cpp')
OBJS := $(patsubst $(SRCDIR)/%.cpp,$(OBJDIR)/%.o,$(SRCS))
INCLUDES := -I$(TOP)/include
LIBS := $(patsubst %,$(LIB_DIR)/lib%.a,$(MODULES))

PUBLIC_DIR := $(BUILD_DIR)/public
DATA_DIR := $(BUILD_DIR)/data

.PHONY: all clean install
all: $(PUBLIC_DIR) $(DATA_DIR) $(BIN_DIR)/$(APP)

$(OBJDIR)/%.o: $(SRCDIR)/%.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

$(BIN_DIR)/$(APP): $(OBJS) $(LIBS)
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) $(LDFLAGS) -o $@ $(OBJS) $(LIBS)


$(PUBLIC_DIR):
	@mkdir -p $@

$(DATA_DIR):
	@mkdir -p $@

clean:
	rm -rf $(OBJDIR) $(BIN_DIR)/$(APP)

install: all
	install -d $(DESTDIR)$(PREFIX)/bin
	install -m 0755 $(BIN_DIR)/$(APP) $(DESTDIR)$(PREFIX)/bin/
