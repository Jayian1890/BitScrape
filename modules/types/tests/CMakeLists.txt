set(TEST_MODULE_NAME ${MODULE_NAME}_tests)

# Source files
file(GLOB_RECURSE TEST_SOURCES "unit/*.cpp")

# Create test executable
add_executable(${TEST_MODULE_NAME} ${TEST_SOURCES})

# Set include directories
target_include_directories(${TEST_MODULE_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

# Set dependencies
target_link_libraries(${TEST_MODULE_NAME} PRIVATE
    ${MODULE_NAME}
    GTest::gtest_main
    GTest::gmock
)

# Set compile options for debug build
target_compile_options(${TEST_MODULE_NAME} PRIVATE
    $<$<CONFIG:Debug>:-g -O0 -Wall -Wextra -Wpedantic>
)

# Add test to CTest
include(GoogleTest)
gtest_discover_tests(${TEST_MODULE_NAME})
