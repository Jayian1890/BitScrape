# Define the library
add_library(bitscrape-beacon
    src/beacon_sink.cpp
    src/console_sink.cpp
    src/file_sink.cpp
    src/event_sink.cpp
    src/beacon.cpp
    src/beacon_adapter.cpp
)

# Set include directories
target_include_directories(bitscrape-beacon
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

# Link dependencies
target_link_libraries(bitscrape-beacon
    PUBLIC
        bitscrape::types
        bitscrape::event
        Threads::Threads
)

# Set target properties
set_target_properties(bitscrape-beacon PROPERTIES
    OUTPUT_NAME bitscrape-beacon
    EXPORT_NAME beacon
)

# Install targets
install(TARGETS bitscrape-beacon
    EXPORT bitscrape-targets
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
    RUNTIME DESTINATION bin
    INCLUDES DESTINATION include
)

# Install headers
install(DIRECTORY include/bitscrape/beacon
    DESTINATION include/bitscrape
    FILES_MATCHING PATTERN "*.hpp"
)

# Add alias target
add_library(bitscrape::beacon ALIAS bitscrape-beacon)

# Tests
if(BUILD_TESTS)
    # Create test executable
    add_executable(beacon_test
        tests/unit/beacon_sink_test.cpp
        tests/unit/console_sink_test.cpp
        tests/unit/file_sink_test.cpp
        tests/unit/event_sink_test.cpp
        tests/unit/beacon_test.cpp
        tests/unit/beacon_adapter_test.cpp
    )

    # Link dependencies
    target_link_libraries(beacon_test
        PRIVATE
            bitscrape-beacon
            GTest::gtest
            GTest::gtest_main
    )

    # Discover tests
    gtest_discover_tests(beacon_test
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    )
endif()
