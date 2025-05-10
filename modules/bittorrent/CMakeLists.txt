# Define the library
add_library(bitscrape-bittorrent
    src/peer_connection.cpp
    src/peer_message.cpp
    src/peer_wire_protocol.cpp
    src/metadata_exchange.cpp
    src/peer_manager.cpp
    src/bittorrent_event_processor.cpp
)

# If files don't exist yet, create empty files
file(TOUCH ${CMAKE_CURRENT_SOURCE_DIR}/src/peer_connection.cpp)
file(TOUCH ${CMAKE_CURRENT_SOURCE_DIR}/src/peer_message.cpp)
file(TOUCH ${CMAKE_CURRENT_SOURCE_DIR}/src/peer_wire_protocol.cpp)
file(TOUCH ${CMAKE_CURRENT_SOURCE_DIR}/src/metadata_exchange.cpp)
file(TOUCH ${CMAKE_CURRENT_SOURCE_DIR}/src/peer_manager.cpp)
file(TOUCH ${CMAKE_CURRENT_SOURCE_DIR}/src/bittorrent_event_processor.cpp)

# Set include directories
target_include_directories(bitscrape-bittorrent
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

# Link dependencies
target_link_libraries(bitscrape-bittorrent
    PUBLIC
        bitscrape::types
        bitscrape::event
        bitscrape::bencode
        bitscrape::network
        Threads::Threads
)

# Set target properties
set_target_properties(bitscrape-bittorrent PROPERTIES
    OUTPUT_NAME bitscrape-bittorrent
    EXPORT_NAME bittorrent
)

# Install targets
install(TARGETS bitscrape-bittorrent
    EXPORT bitscrape-targets
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
    RUNTIME DESTINATION bin
    INCLUDES DESTINATION include
)

# Install headers
install(DIRECTORY include/bitscrape/bittorrent
    DESTINATION include/bitscrape
    FILES_MATCHING PATTERN "*.hpp"
)

# Add alias target
add_library(bitscrape::bittorrent ALIAS bitscrape-bittorrent)

# Tests
if(BUILD_TESTS)
    # Create test executable
    add_executable(bittorrent_test
        tests/unit/peer_connection_test.cpp
        tests/unit/peer_message_test.cpp
        tests/unit/peer_wire_protocol_test.cpp
        tests/unit/metadata_exchange_test.cpp
        tests/unit/peer_manager_test.cpp
        tests/unit/bittorrent_event_processor_test.cpp
    )

    # If test files don't exist yet, create empty files
    file(TOUCH ${CMAKE_CURRENT_SOURCE_DIR}/tests/unit/peer_connection_test.cpp)
    file(TOUCH ${CMAKE_CURRENT_SOURCE_DIR}/tests/unit/peer_message_test.cpp)
    file(TOUCH ${CMAKE_CURRENT_SOURCE_DIR}/tests/unit/peer_wire_protocol_test.cpp)
    file(TOUCH ${CMAKE_CURRENT_SOURCE_DIR}/tests/unit/metadata_exchange_test.cpp)
    file(TOUCH ${CMAKE_CURRENT_SOURCE_DIR}/tests/unit/peer_manager_test.cpp)
    file(TOUCH ${CMAKE_CURRENT_SOURCE_DIR}/tests/unit/bittorrent_event_processor_test.cpp)

    # Link dependencies
    target_link_libraries(bittorrent_test
        PRIVATE
            bitscrape-bittorrent
            GTest::gtest
            GTest::gtest_main
    )

    # Discover tests
    gtest_discover_tests(bittorrent_test
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    )
endif()
