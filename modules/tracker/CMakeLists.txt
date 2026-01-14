# Tracker module - depends on types, bencode, network
file(GLOB_RECURSE TRACKER_SOURCES src/*.cpp)
add_library(tracker STATIC ${TRACKER_SOURCES})
target_include_directories(tracker PUBLIC ${CMAKE_SOURCE_DIR}/include)
target_link_libraries(tracker PUBLIC types bencode network)

