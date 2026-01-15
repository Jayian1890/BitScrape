cmake_minimum_required(VERSION 3.20)
project(BitScrape VERSION 1.0.0 LANGUAGES CXX)

# Set C++20 standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Export compile_commands.json for IDE integration
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Detect and prefer Homebrew LLVM on macOS
if(APPLE AND NOT DEFINED CMAKE_CXX_COMPILER)
    if(EXISTS "/opt/homebrew/opt/llvm/bin/clang++")
        set(CMAKE_CXX_COMPILER "/opt/homebrew/opt/llvm/bin/clang++")
        set(CMAKE_C_COMPILER "/opt/homebrew/opt/llvm/bin/clang")
        message(STATUS "Using Homebrew LLVM: ${CMAKE_CXX_COMPILER}")

        # Explicitly set paths for include directories and sysroot
        set(CMAKE_SYSROOT "/Library/Developer/CommandLineTools/SDKs/MacOSX.sdk")
        include_directories("/opt/homebrew/opt/llvm/include/c++/v1")
        link_directories("/opt/homebrew/opt/llvm/lib")
    endif()
endif()

# Additional Compiler flags
add_compile_options(
    -Wall
    -Wextra
    -Wpedantic
)

# Use libc++ on macOS with Homebrew LLVM
if(APPLE AND CMAKE_CXX_COMPILER MATCHES ".*homebrew.*")
    add_compile_options(-stdlib=libc++)
    add_link_options(-stdlib=libc++)
    add_link_options("-L/opt/homebrew/opt/llvm/lib/c++")
    add_link_options("-Wl,-rpath,/opt/homebrew/opt/llvm/lib/c++")
endif()

# Build type specific flags
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    add_compile_options(-g -O0)
else()
    add_compile_options(-O2)
endif()

# Global include directory
include_directories(${CMAKE_SOURCE_DIR}/include)

# Add modules as static libraries
add_subdirectory(modules/types)
add_subdirectory(modules/beacon)
add_subdirectory(modules/bencode)
add_subdirectory(modules/event)
add_subdirectory(modules/lock)
add_subdirectory(modules/network)
add_subdirectory(modules/storage)
add_subdirectory(modules/dht)
add_subdirectory(modules/tracker)
add_subdirectory(modules/bittorrent)
add_subdirectory(modules/core)

# Add applications
add_subdirectory(apps/web)
add_subdirectory(apps/cli)

# Create output directories
file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/public)
file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/data)

# Install public directory
install(DIRECTORY public DESTINATION share/bitscrape)

# Print configuration summary
message(STATUS "")
message(STATUS "BitScrape Configuration:")
message(STATUS "  Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "  C++ compiler: ${CMAKE_CXX_COMPILER}")
message(STATUS "  C++ standard: C++${CMAKE_CXX_STANDARD}")
message(STATUS "  Coverage: ${ENABLE_COVERAGE}")
message(STATUS "")