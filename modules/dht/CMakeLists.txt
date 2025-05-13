# Define the library
add_library(bitscrape-dht
        src/dht_session.cpp
        src/routing_table.cpp
        src/bootstrap.cpp
        src/node_lookup.cpp
        src/dht_message.cpp
        src/dht_message_factory.cpp
        src/token_manager.cpp
        src/k_bucket.cpp
        src/dht_get_peers_message.cpp
        src/dht_find_node_message.cpp
        src/dht_announce_peer_message.cpp
)

# Set include directories
target_include_directories(bitscrape-dht
        PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

# Link dependencies
target_link_libraries(bitscrape-dht
        PUBLIC
        bitscrape::types
        PRIVATE
        bitscrape::event
        bitscrape::network
        bitscrape::bencode
)

# Set target properties
set_target_properties(bitscrape-dht PROPERTIES
        OUTPUT_NAME bitscrape-dht
        EXPORT_NAME dht
)

# Install targets
install(TARGETS bitscrape-dht
        EXPORT bitscrape-targets
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib
        RUNTIME DESTINATION bin
        INCLUDES DESTINATION include
)

# Install headers
install(DIRECTORY include/bitscrape/dht
        DESTINATION include/bitscrape
        FILES_MATCHING PATTERN "*.hpp"
)

# Add alias target
add_library(bitscrape::dht ALIAS bitscrape-dht)

# Tests
if (BUILD_TESTS)
    # Create test executable
    add_executable(dht_test
            tests/unit/dht_session_test.cpp
            tests/unit/routing_table_test.cpp
            tests/unit/bootstrap_test.cpp
            tests/unit/node_lookup_test.cpp
            tests/unit/dht_message_test.cpp
            tests/unit/dht_message_factory_test.cpp
            tests/unit/token_manager_test.cpp
    )

    # Link dependencies
    target_link_libraries(dht_test
            PRIVATE
            bitscrape-dht
            bitscrape-event
            bitscrape-network
            bitscrape-bencode
            bitscrape-gtest
    )

    # Add to protocol tests group
    add_dependencies(protocol_tests dht_test)
endif ()
