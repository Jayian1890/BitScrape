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
    lock
)

# Set compile options for debug build
target_compile_options(${APP_NAME} PRIVATE
    $<$<CONFIG:Debug>:-g -O0 -Wall -Wextra -Wpedantic>
)

# Create an empty configuration file template in the build directory
file(WRITE ${CMAKE_BINARY_DIR}/bitscrape.json.template "{}")

# Create public directory for web interface
file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/public)

# Create data directory for database
file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/data)
