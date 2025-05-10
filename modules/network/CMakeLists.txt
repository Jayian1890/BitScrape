# Define the library
add_library(bitscrape-network
    src/socket.cpp
    src/address.cpp
    src/buffer.cpp
    src/udp_socket.cpp
    src/tcp_socket.cpp
    src/tcp_listener.cpp
    src/http_client.cpp
    src/nat_traversal.cpp
)

# Set include directories
target_include_directories(bitscrape-network
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

# Link dependencies
target_link_libraries(bitscrape-network
    PUBLIC
        bitscrape::types
        Threads::Threads
)

# Set target properties
set_target_properties(bitscrape-network PROPERTIES
    OUTPUT_NAME bitscrape-network
    EXPORT_NAME network
)

# Install targets
install(TARGETS bitscrape-network
    EXPORT bitscrape-targets
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
    RUNTIME DESTINATION bin
    INCLUDES DESTINATION include
)

# Install headers
install(DIRECTORY include/bitscrape/network
    DESTINATION include/bitscrape
    FILES_MATCHING PATTERN "*.hpp"
)

# Add alias target
add_library(bitscrape::network ALIAS bitscrape-network)

# Tests
if(BUILD_TESTS)
    # Create test executable
    add_executable(network_test
        tests/unit/socket_test.cpp
        tests/unit/address_test.cpp
        tests/unit/buffer_test.cpp
    )

    # Link dependencies
    target_link_libraries(network_test
        PRIVATE
            bitscrape-network
            GTest::gtest
            GTest::gtest_main
    )

    # Discover tests
    gtest_discover_tests(network_test
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    )
endif()
