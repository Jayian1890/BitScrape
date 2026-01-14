# CLI Application
add_executable(bitscrape_cli main.cpp)

# Link against all module libraries
target_link_libraries(bitscrape_cli PRIVATE
    core
    web
)

target_include_directories(bitscrape_cli PRIVATE ${CMAKE_SOURCE_DIR}/include)

# Set output directory
set_target_properties(bitscrape_cli PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)

# Install target
install(TARGETS bitscrape_cli DESTINATION bin)

# Copy public directory to build directory for local running
add_custom_command(TARGET bitscrape_cli POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/public
        $<TARGET_FILE_DIR:bitscrape_cli>/public
    COMMENT "Copying public directory to build output"
)
