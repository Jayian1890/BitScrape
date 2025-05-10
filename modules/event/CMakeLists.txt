# Define the library
add_library(bitscrape-event
    src/event_bus.cpp
    src/event_processor.cpp
    src/event_adapter.cpp
    src/event_filter.cpp
)

# Set include directories
target_include_directories(bitscrape-event
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

# Link dependencies
target_link_libraries(bitscrape-event
    PUBLIC
        bitscrape::types
        Threads::Threads
)

# Set target properties
set_target_properties(bitscrape-event PROPERTIES
    OUTPUT_NAME bitscrape-event
    EXPORT_NAME event
)

# Install targets
install(TARGETS bitscrape-event
    EXPORT bitscrape-targets
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
    RUNTIME DESTINATION bin
    INCLUDES DESTINATION include
)

# Install headers
install(DIRECTORY include/bitscrape/event
    DESTINATION include/bitscrape
    FILES_MATCHING PATTERN "*.hpp"
)

# Add alias target
add_library(bitscrape::event ALIAS bitscrape-event)

# Tests
if(BUILD_TESTS)
    # Create test executable
    add_executable(event_test
        tests/unit/event_bus_test.cpp
        tests/unit/event_processor_test.cpp
        tests/unit/event_adapter_test.cpp
        tests/unit/event_filter_test.cpp
    )

    # Link dependencies
    target_link_libraries(event_test
        PRIVATE
            bitscrape-event
            bitscrape-gtest
    )

    # Add to foundation tests group
    add_dependencies(foundation_tests event_test)
endif()
