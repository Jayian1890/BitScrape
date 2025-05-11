# Define the library
add_library(bitscrape-core
    src/controller.cpp
    src/configuration.cpp
)

# Set include directories
target_include_directories(bitscrape-core
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

# Link dependencies
target_link_libraries(bitscrape-core
    PUBLIC
        bitscrape::types
    PRIVATE
        bitscrape::event
        bitscrape::beacon
        bitscrape::network
        bitscrape::bencode
        bitscrape::bittorrent
        bitscrape::tracker
        bitscrape::storage
)

# Set target properties
set_target_properties(bitscrape-core PROPERTIES
    OUTPUT_NAME bitscrape-core
    EXPORT_NAME core
)

# Install targets
install(TARGETS bitscrape-core
    EXPORT bitscrape-targets
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
    RUNTIME DESTINATION bin
    INCLUDES DESTINATION include
)

# Install headers
install(DIRECTORY include/bitscrape/core
    DESTINATION include/bitscrape
    FILES_MATCHING PATTERN "*.hpp"
)

# Add alias target
add_library(bitscrape::core ALIAS bitscrape-core)

# Tests
if(BUILD_TESTS)
    # Create test executable
    add_executable(core_test
        tests/unit/controller_test.cpp
        tests/unit/configuration_test.cpp
    )

    # Link dependencies
    target_link_libraries(core_test
        PRIVATE
            bitscrape-core
            bitscrape-event
            bitscrape-beacon
            bitscrape-network
            bitscrape-bencode
            bitscrape-bittorrent
            bitscrape-tracker
            bitscrape-storage
            bitscrape-gtest
    )

    # Add to core tests group
    add_dependencies(foundation_tests core_test)
endif()
