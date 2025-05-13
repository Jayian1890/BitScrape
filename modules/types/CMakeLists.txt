# Define the library
add_library(bitscrape-types
    src/node_id.cpp
    src/info_hash.cpp
    src/endpoint.cpp
    src/event_types.cpp
    src/message_types.cpp
    src/dht_node.cpp
    src/dht_token.cpp
    src/dht_routing_table_entry.cpp
    src/metadata_info.cpp
    src/metadata_piece.cpp
    src/torrent_info.cpp
    src/beacon_types.cpp
)

# Set include directories
target_include_directories(bitscrape-types
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

# Set target properties
set_target_properties(bitscrape-types PROPERTIES
    OUTPUT_NAME bitscrape-types
    EXPORT_NAME types
)

# Install targets
install(TARGETS bitscrape-types
    EXPORT bitscrape-targets
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
    RUNTIME DESTINATION bin
    INCLUDES DESTINATION include
)

# Install headers
install(DIRECTORY include/bitscrape/types
    DESTINATION include/bitscrape
    FILES_MATCHING PATTERN "*.hpp"
)

# No external dependencies

# Add alias target
add_library(bitscrape::types ALIAS bitscrape-types)

# Tests
if(BUILD_TESTS)
    # Create test executable
    add_executable(types_test
        tests/unit/node_id_test.cpp
        tests/unit/info_hash_test.cpp
        tests/unit/endpoint_test.cpp
        tests/unit/event_types_test.cpp
        tests/unit/message_types_test.cpp
        tests/unit/dht_node_test.cpp
        tests/unit/dht_token_test.cpp
        tests/unit/dht_routing_table_entry_test.cpp
        tests/unit/metadata_info_test.cpp
        tests/unit/metadata_piece_test.cpp
        tests/unit/torrent_info_test.cpp
        tests/unit/beacon_types_test.cpp
    )

    # Link dependencies
    target_link_libraries(types_test
        PRIVATE
            bitscrape-types
            bitscrape-gtest
    )

    # Add to foundation tests group
    add_dependencies(foundation_tests types_test)
endif()
