# CLI application
if(BUILD_CLI)
    add_executable(bitscrape_cli
        cli/src/main.cpp
    )

    target_link_libraries(bitscrape_cli
        PRIVATE
            bitscrape::types
            bitscrape::event
            bitscrape::beacon
            bitscrape::core
            bitscrape::storage
    )

    # Link with web module if enabled
    if (ENABLE_WEB_INTERFACE)
        target_link_libraries(bitscrape_cli
                PRIVATE
                bitscrape::web
        )
        target_compile_definitions(bitscrape_cli
                PRIVATE
                ENABLE_WEB_INTERFACE
        )

        # Copy static files to build directory
        file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/web/public/ DESTINATION ${CMAKE_BINARY_DIR}/bin/public)
    endif ()

    install(TARGETS bitscrape_cli
        RUNTIME DESTINATION bin
    )

    # Install static files if web interface is enabled
    if (ENABLE_WEB_INTERFACE)
        install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/web/public/
                DESTINATION share/bitscrape/public
        )
    endif ()
endif()

# GUI application
if(BUILD_GUI)
    add_subdirectory(gui)
endif()

# Web application
if (ENABLE_WEB_INTERFACE AND BUILD_WEB_APP)
    add_subdirectory(web)
endif ()
