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

    install(TARGETS bitscrape_cli
        RUNTIME DESTINATION bin
    )
endif()

# GUI application
if(BUILD_GUI)
    add_subdirectory(gui)
endif()
