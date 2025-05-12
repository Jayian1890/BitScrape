# Web application
add_executable(bitscrape_web
        src/main.cpp
)

target_link_libraries(bitscrape_web
        PRIVATE
        bitscrape::types
        bitscrape::event
        bitscrape::beacon
        bitscrape::core
        bitscrape::storage
        bitscrape::web
)

# Copy static files to build directory
file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/public/ DESTINATION ${CMAKE_BINARY_DIR}/bin/public)

# Install executable
install(TARGETS bitscrape_web
        RUNTIME DESTINATION bin
)

# Install static files
install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/public/
        DESTINATION share/bitscrape/public
)
