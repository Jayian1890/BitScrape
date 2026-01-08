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

TEMPLATE := $(BUILD_DIR)/bitscrape.json.template
PUBLIC_DIR := $(BUILD_DIR)/public
DATA_DIR := $(BUILD_DIR)/data

.PHONY: all clean install
all: $(TEMPLATE) $(PUBLIC_DIR) $(DATA_DIR) $(BIN_DIR)/$(APP)

$(OBJDIR)/%.o: $(SRCDIR)/%.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

$(BIN_DIR)/$(APP): $(OBJS) $(LIBS)
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -o $@ $(OBJS) $(LIBS)

$(TEMPLATE):
	@mkdir -p $(dir $@)
	@if [ ! -f $@ ]; then \
		cat > $@ <<'EOF'; \
	{
	  "database.path": "bitscrape.db",
	  "dht.bootstrap_nodes": [
	    "v4.router.mega.co.nz:6881",
	    "dht.aelitis.com:6881",
	    "router.utorrent.com:6881",
	    "router.bittorrent.com:6881"
	  ],
	  "dht.bootstrap_infohash": "",
	  "dht.bootstrap_trackers": [
	    "udp://tracker.opentrackr.org:1337/announce",
	    "udp://tracker.openbittorrent.com:6969/announce"
	  ],
	  "dht.port": 6881,
	  "dht.node_id": "",
	  "dht.max_nodes": 1000,
	  "dht.ping_interval": 300,
	  "bittorrent.max_connections": 50,
	  "bittorrent.connection_timeout": 10,
	  "bittorrent.download_timeout": 30,
	  "tracker.announce_interval": 1800,
	  "tracker.max_trackers": 20,
	  "log.level": "info",
	  "log.file": "bitscrape.log",
	  "log.max_size": 10485760,
	  "log.max_files": 5,
	  "web.auto_start": true,
	  "web.port": 8080,
	  "web.static_dir": "public"
	}
	EOF \
	; fi

$(PUBLIC_DIR):
	@mkdir -p $@

$(DATA_DIR):
	@mkdir -p $@

clean:
	rm -rf $(OBJDIR) $(BIN_DIR)/$(APP)

install: all
	install -d $(DESTDIR)$(PREFIX)/bin
	install -m 0755 $(BIN_DIR)/$(APP) $(DESTDIR)$(PREFIX)/bin/
