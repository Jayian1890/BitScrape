# CLI Application
file(GLOB_RECURSE CLI_SOURCES src/*.cpp)
add_executable(bitscrape_cli ${CLI_SOURCES})

# Link against all module libraries
target_link_libraries(bitscrape_cli PRIVATE
    core
    lock
    types
    beacon
    event
    storage
    dht
    bittorrent
    tracker
    network
    bencode
    web
)

target_include_directories(bitscrape_cli PRIVATE ${CMAKE_SOURCE_DIR}/include)

# Set output directory
set_target_properties(bitscrape_cli PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)

# Install target
install(TARGETS bitscrape_cli DESTINATION bin)
