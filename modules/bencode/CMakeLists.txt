# Define the library
add_library(bitscrape-bencode
    src/bencode_value.cpp
    src/bencode_encoder.cpp
    src/bencode_decoder.cpp
    src/bencode_stream.cpp
    src/bencode_event_processor.cpp
)

# Set include directories
target_include_directories(bitscrape-bencode
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

# Link dependencies
target_link_libraries(bitscrape-bencode
    PUBLIC
        bitscrape::types
        bitscrape::event
)

# Set target properties
set_target_properties(bitscrape-bencode PROPERTIES
    OUTPUT_NAME bitscrape-bencode
    EXPORT_NAME bencode
)

# Install targets
install(TARGETS bitscrape-bencode
    EXPORT bitscrape-targets
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
    RUNTIME DESTINATION bin
    INCLUDES DESTINATION include
)

# Install headers
install(DIRECTORY include/bitscrape/bencode
    DESTINATION include/bitscrape
    FILES_MATCHING PATTERN "*.hpp"
)

# Add alias target
add_library(bitscrape::bencode ALIAS bitscrape-bencode)

# Tests
if(BUILD_TESTS)
    # Create test executable
    add_executable(bencode_test
        tests/unit/bencode_value_test.cpp
        tests/unit/bencode_encoder_test.cpp
        tests/unit/bencode_decoder_test.cpp
        tests/unit/bencode_stream_test.cpp
        tests/unit/bencode_event_processor_test.cpp
    )

    # Link dependencies
    target_link_libraries(bencode_test
        PRIVATE
            bitscrape-bencode
            GTest::gtest
            GTest::gtest_main
    )

    # Discover tests
    gtest_discover_tests(bencode_test
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    )
endif()
