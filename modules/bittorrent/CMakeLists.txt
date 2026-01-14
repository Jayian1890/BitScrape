# BitTorrent module - depends on types, bencode, network, beacon, event
file(GLOB_RECURSE BITTORRENT_SOURCES src/*.cpp)
add_library(bittorrent STATIC ${BITTORRENT_SOURCES})
target_include_directories(bittorrent PUBLIC ${CMAKE_SOURCE_DIR}/include)
target_link_libraries(bittorrent PUBLIC types bencode network beacon event)

