cmake_minimum_required(VERSION 3.14)
project(BitScrape VERSION 0.1.0 LANGUAGES CXX)

cmake_policy(SET CMP0135 NEW)

# Set C++ standard
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Debug build configuration
set(CMAKE_BUILD_TYPE Debug)
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -g -O0 -Wall -Wextra -Wpedantic")

# Include directories
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/modules/beacon/include
        ${CMAKE_CURRENT_SOURCE_DIR}/modules/bencode/include
        ${CMAKE_CURRENT_SOURCE_DIR}/modules/bittorrent/include
    ${CMAKE_CURRENT_SOURCE_DIR}/modules/core/include
        ${CMAKE_CURRENT_SOURCE_DIR}/modules/dht/include
        ${CMAKE_CURRENT_SOURCE_DIR}/modules/event/include
        ${CMAKE_CURRENT_SOURCE_DIR}/modules/network/include
    ${CMAKE_CURRENT_SOURCE_DIR}/modules/storage/include
        ${CMAKE_CURRENT_SOURCE_DIR}/modules/tracker/include
        ${CMAKE_CURRENT_SOURCE_DIR}/modules/types/include
        ${CMAKE_CURRENT_SOURCE_DIR}/modules/web/include
)

# Add GoogleTest
include(FetchContent)
FetchContent_Declare(
        googletest
        URL https://github.com/google/googletest/archive/refs/tags/v1.14.0.zip
)
# For Windows: Prevent overriding the parent project's compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

# Enable testing
enable_testing()

# Define module libraries
add_subdirectory(modules/types)
add_subdirectory(modules/event)
add_subdirectory(modules/beacon)
add_subdirectory(modules/network)
add_subdirectory(modules/storage)
add_subdirectory(modules/bencode)
add_subdirectory(modules/dht)
add_subdirectory(modules/bittorrent)
add_subdirectory(modules/tracker)
add_subdirectory(modules/core)
add_subdirectory(modules/web)

# Define CLI application
add_subdirectory(apps/cli)

# Install targets
install(TARGETS bitscrape_cli
        RUNTIME DESTINATION bin
)

# Install configuration file
install(FILES bitscrape.conf
        DESTINATION etc
)

# Create a target for running the CLI application
add_custom_target(run_cli
        COMMAND bitscrape_cli
        DEPENDS bitscrape_cli
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
)
