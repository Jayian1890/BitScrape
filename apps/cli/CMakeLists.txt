set(APP_NAME bitscrape_cli)

# Source files
file(GLOB_RECURSE SOURCES "src/*.cpp")

# Create executable
add_executable(${APP_NAME} ${SOURCES})

# Set include directories
target_include_directories(${APP_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

# Set dependencies
target_link_libraries(${APP_NAME} PRIVATE
    types
    event
    beacon
    network
    storage
    bencode
    dht
    bittorrent
    tracker
    core
    web
)

# Set compile options for debug build
target_compile_options(${APP_NAME} PRIVATE
    $<$<CONFIG:Debug>:-g -O0 -Wall -Wextra -Wpedantic>
)

# Copy configuration file to build directory
configure_file(
    ${CMAKE_SOURCE_DIR}/bitscrape.conf
    ${CMAKE_BINARY_DIR}/bitscrape.conf
    COPYONLY
)

# Create public directory for web interface
file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/public)

# Create data directory for database
file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/data)
