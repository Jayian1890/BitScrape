CXX = g++
CXXFLAGS = -std=c++23 -Wall -Wextra -I../../modules/types/include -I../../modules/storage/include
LDFLAGS = -lsqlite3 -pthread

SRCS = src/simple_cli.cpp \
       ../../modules/storage/src/storage_manager.cpp \
       ../../modules/storage/src/database.cpp \
       ../../modules/storage/src/data_models.cpp \
       ../../modules/storage/src/query_interface.cpp \
       ../../modules/storage/src/migration_manager.cpp \
       ../../modules/types/src/node_id.cpp \
       ../../modules/types/src/info_hash.cpp \
       ../../modules/types/src/endpoint.cpp \
       ../../modules/types/src/metadata_info.cpp

OBJS = $(SRCS:.cpp=.o)
TARGET = storage_cli

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(TARGET)
