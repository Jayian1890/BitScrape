TOP ?= $(abspath $(TOP))
BUILD_DIR ?= build
BIN_DIR ?= $(BUILD_DIR)/bin
LIB_DIR ?= $(BUILD_DIR)/lib
CXX ?= g++
CXXFLAGS ?= -std=c++23 -Wall -Wextra -Wpedantic
MODULES ?= $(MODULES)

APP := bitscrape_cli
SRCDIR := src
OBJDIR := $(BUILD_DIR)/apps/cli
SRCS := $(shell find $(SRCDIR) -name '*.cpp')
OBJS := $(patsubst $(SRCDIR)/%.cpp,$(OBJDIR)/%.o,$(SRCS))
INCLUDES := -Iinclude $(shell for d in $(TOP)/modules/*/include; do if [ -d $$d ]; then printf " -I%s" $$d; fi; done)
LIBS := $(patsubst %,$(LIB_DIR)/lib%.a,$(MODULES))

.PHONY: all clean install

all: $(BIN_DIR)/$(APP)

$(OBJDIR)/%.o: $(SRCDIR)/%.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

$(BIN_DIR)/$(APP): $(OBJS) $(LIBS)
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -o $@ $(OBJS) $(LIBS)

clean:
	rm -rf $(OBJDIR) $(BIN_DIR)/$(APP)

install: all
	install -d $(DESTDIR)$(PREFIX)/bin
	install -m 0755 $(BIN_DIR)/$(APP) $(DESTDIR)$(PREFIX)/bin/
