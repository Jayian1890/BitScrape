# Define the library
add_library(bitscrape-tracker
    src/tracker_request.cpp
    src/tracker_response.cpp
    src/http_tracker.cpp
    src/udp_tracker.cpp
    src/tracker_manager.cpp
    src/tracker_event_processor.cpp
    src/tracker_event.cpp
    src/tracker_announce.cpp
    src/tracker_scrape.cpp
)

# Set include directories
target_include_directories(bitscrape-tracker
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

# Link dependencies
target_link_libraries(bitscrape-tracker
    PUBLIC
        bitscrape::types
    PRIVATE
        bitscrape::event
        bitscrape::network
        bitscrape::bencode
        bitscrape::beacon
)

# Set target properties
set_target_properties(bitscrape-tracker PROPERTIES
    OUTPUT_NAME bitscrape-tracker
    EXPORT_NAME tracker
)

# Install targets
install(TARGETS bitscrape-tracker
    EXPORT bitscrape-targets
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
    RUNTIME DESTINATION bin
    INCLUDES DESTINATION include
)

# Install headers
install(DIRECTORY include/bitscrape/tracker
    DESTINATION include/bitscrape
    FILES_MATCHING PATTERN "*.hpp"
)

# Add alias target
add_library(bitscrape::tracker ALIAS bitscrape-tracker)

# Tests
if(BUILD_TESTS)
    # Create test executable
    add_executable(tracker_test
        tests/unit/tracker_request_test.cpp
        tests/unit/tracker_response_test.cpp
        tests/unit/http_tracker_test.cpp
        tests/unit/udp_tracker_test.cpp
        tests/unit/tracker_manager_test.cpp
        tests/unit/tracker_event_processor_test.cpp
        tests/unit/tracker_event_test.cpp
        tests/unit/tracker_announce_test.cpp
        tests/unit/tracker_scrape_test.cpp
    )

    # Link dependencies
    target_link_libraries(tracker_test
        PRIVATE
            bitscrape-tracker
            bitscrape-event
            bitscrape-network
            bitscrape-bencode
            bitscrape-beacon
            bitscrape-gtest
    )

    # Add to protocol tests group
    add_dependencies(protocol_tests tracker_test)
endif()

# Source files have been created manually
