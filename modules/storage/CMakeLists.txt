# Define the library
add_library(bitscrape-storage
    src/storage_manager.cpp
    src/database.cpp
    src/data_models.cpp
    src/query_interface.cpp
    src/migration_manager.cpp
)

# Set include directories
target_include_directories(bitscrape-storage
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

# Link dependencies
target_link_libraries(bitscrape-storage
    PUBLIC
        bitscrape::types
        bitscrape::event
    PRIVATE
        bitscrape::beacon
        SQLite::SQLite3
        Threads::Threads
)

# Set target properties
set_target_properties(bitscrape-storage PROPERTIES
    OUTPUT_NAME bitscrape-storage
    EXPORT_NAME storage
)

# Install targets
install(TARGETS bitscrape-storage
    EXPORT bitscrape-targets
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
    RUNTIME DESTINATION bin
    INCLUDES DESTINATION include
)

# Install headers
install(DIRECTORY include/bitscrape/storage
    DESTINATION include/bitscrape
    FILES_MATCHING PATTERN "*.hpp"
)

# Add alias target
add_library(bitscrape::storage ALIAS bitscrape-storage)

# Tests
if(BUILD_TESTS)
    # Create test executable
    add_executable(storage_test
        tests/unit/storage_manager_test.cpp
        tests/unit/database_test.cpp
        tests/unit/data_models_test.cpp
        tests/unit/query_interface_test.cpp
        tests/unit/migration_manager_test.cpp
    )

    # Link dependencies
    target_link_libraries(storage_test
        PRIVATE
            bitscrape-storage
            bitscrape-gtest
    )

    # Add to foundation tests group
    add_dependencies(foundation_tests storage_test)
endif()
