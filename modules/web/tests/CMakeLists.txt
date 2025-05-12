# Create test executable
add_executable(web_test
        unit/http_server_test.cpp
        unit/http_router_test.cpp
        unit/api_handler_test.cpp
        unit/web_controller_test.cpp
)

# Link dependencies
target_link_libraries(web_test
        PRIVATE
        bitscrape-web
        bitscrape-core
        bitscrape-storage
        bitscrape-gtest
)

# Add to foundation tests group
add_dependencies(foundation_tests web_test)
