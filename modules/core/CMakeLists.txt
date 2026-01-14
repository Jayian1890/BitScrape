# Core module - depends on all other modules
file(GLOB_RECURSE CORE_SOURCES src/*.cpp)
add_library(core STATIC ${CORE_SOURCES})
target_include_directories(core PUBLIC ${CMAKE_SOURCE_DIR}/include)
target_link_libraries(core PUBLIC 
    types
    beacon
    bencode
    event
    lock
    network
    storage
    dht
    tracker
    bittorrent
)

