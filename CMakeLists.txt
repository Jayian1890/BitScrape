cmake_minimum_required(VERSION 3.20)
project(bitscrape VERSION 0.1.0 LANGUAGES CXX)

# Set C++23 standard
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Set output directories
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# Add compiler flags
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    add_compile_options(
        -Wno-unused-parameter
        -Wno-unused-variable
        -Wno-missing-field-initializers
    )
    if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        add_compile_options(-D_LIBCPP_HARDENING_MODE=_LIBCPP_HARDENING_MODE_FAST)
    endif()
endif()

# Options
option(BUILD_TESTS "Build tests" ON)
option(ENABLE_ASAN "Enable AddressSanitizer" OFF)
option(BUILD_TOOLS "Build tools" ON)
option(BUILD_EXAMPLES "Build examples" ON)
option(BUILD_GUI "Build GUI application" ON)
option(BUILD_CLI "Build CLI application" ON)
option(BUILD_DOCS "Build documentation" OFF)
option(ENABLE_WEB_INTERFACE "Enable web interface" ON)
option(BUILD_WEB_APP "Build standalone web application" OFF)
option(ENABLE_NAT_TRAVERSAL "Enable NAT traversal" ON)

# Enable AddressSanitizer if requested
if(ENABLE_ASAN)
    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        add_compile_options(-fsanitize=address)
        add_link_options(-fsanitize=address)
    endif()
endif()

# Find required dependencies
find_package(Threads REQUIRED)

# Find SQLite3
include(FetchContent)
# SQLite3 dependency has been removed in favor of a custom key-value store

# Testing setup
if(BUILD_TESTS)
    # Find Google Test
    include(FetchContent)
    FetchContent_Declare(
        googletest
        GIT_REPOSITORY https://github.com/google/googletest.git
        GIT_TAG v1.14.0
    )
    # Configure Google Test options
    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
    set(BUILD_GMOCK OFF CACHE BOOL "" FORCE)
    set(INSTALL_GTEST OFF CACHE BOOL "" FORCE)
    set(BUILD_SHARED_LIBS ON CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(googletest)

    # Set environment variables for tests
    set(ENV{GTEST_COLOR} "1")

    # Create a custom target for Google Test
    add_library(bitscrape-gtest INTERFACE)
    target_link_libraries(bitscrape-gtest INTERFACE gtest_main)

    # Note: There might be a warning about duplicate libraries (libgtest.a) during linking.
    # This is a harmless warning that occurs when multiple test executables link to the same
    # Google Test library. It does not affect the functionality of the tests.

    # Create test groups
    add_custom_target(foundation_tests)
    add_custom_target(protocol_tests)
endif()

# Include directories
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/modules/types/include
    ${CMAKE_CURRENT_SOURCE_DIR}/modules/event/include
    ${CMAKE_CURRENT_SOURCE_DIR}/modules/beacon/include
        ${CMAKE_CURRENT_SOURCE_DIR}/modules/bencode/include
        ${CMAKE_CURRENT_SOURCE_DIR}/modules/network/include
        ${CMAKE_CURRENT_SOURCE_DIR}/modules/dht/include
        ${CMAKE_CURRENT_SOURCE_DIR}/modules/bittorrent/include
        ${CMAKE_CURRENT_SOURCE_DIR}/modules/tracker/include
    ${CMAKE_CURRENT_SOURCE_DIR}/modules/core/include
    ${CMAKE_CURRENT_SOURCE_DIR}/modules/storage/include
        ${CMAKE_CURRENT_SOURCE_DIR}/modules/web/include
)

# Add modules
add_subdirectory(modules/types)
add_subdirectory(modules/event)
add_subdirectory(modules/beacon)
add_subdirectory(modules/bencode)
add_subdirectory(modules/network)
add_subdirectory(modules/dht) # Re-enabled DHT module
add_subdirectory(modules/bittorrent)
add_subdirectory(modules/tracker)
add_subdirectory(modules/core)
add_subdirectory(modules/storage)
add_subdirectory(modules/web)

# Add applications
add_subdirectory(apps)

# Install targets
install(EXPORT bitscrape-targets
    FILE bitscrape-targets.cmake
    NAMESPACE bitscrape::
    DESTINATION lib/cmake/bitscrape
)

# Generate and install package configuration
include(CMakePackageConfigHelpers)
configure_package_config_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/cmake/bitscrape-config.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/bitscrape-config.cmake
    INSTALL_DESTINATION lib/cmake/bitscrape
)
write_basic_package_version_file(
    ${CMAKE_CURRENT_BINARY_DIR}/bitscrape-config-version.cmake
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion
)
install(
    FILES
        ${CMAKE_CURRENT_BINARY_DIR}/bitscrape-config.cmake
        ${CMAKE_CURRENT_BINARY_DIR}/bitscrape-config-version.cmake
    DESTINATION lib/cmake/bitscrape
)
