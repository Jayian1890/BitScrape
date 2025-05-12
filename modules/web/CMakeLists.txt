# Define the library
add_library(bitscrape-web
        src/http_server.cpp
        src/http_router.cpp
        src/api_handler.cpp
        src/websocket_server.cpp
        src/web_controller.cpp
        src/static_file_handler.cpp
)

# Set include directories
target_include_directories(bitscrape-web
        PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

# Link dependencies
target_link_libraries(bitscrape-web
        PUBLIC
        bitscrape::types
        bitscrape::network
        bitscrape::core
        bitscrape::storage
        PRIVATE
        Threads::Threads
)

# Set target properties
set_target_properties(bitscrape-web PROPERTIES
        OUTPUT_NAME bitscrape-web
        EXPORT_NAME web
)

# Add alias
add_library(bitscrape::web ALIAS bitscrape-web)

# Install targets
install(TARGETS bitscrape-web
        EXPORT bitscrape-targets
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib
        RUNTIME DESTINATION bin
        INCLUDES DESTINATION include
)

# Install headers
install(DIRECTORY include/
        DESTINATION include
)

# Add tests if enabled
if (BUILD_TESTING)
    add_subdirectory(tests)
endif ()
