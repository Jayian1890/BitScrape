set(MODULE_NAME lock)

# Source files
file(GLOB_RECURSE SOURCES "src/*.cpp")

# Create library
add_library(${MODULE_NAME} STATIC ${SOURCES})

# Set include directories
target_include_directories(${MODULE_NAME} PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

# Set dependencies
target_link_libraries(${MODULE_NAME} PUBLIC
        types
        beacon
)

# Set compile options for debug build
target_compile_options(${MODULE_NAME} PRIVATE
        $<$<CONFIG:Debug>:-g -O0 -Wall -Wextra -Wpedantic>
)

# Add tests if they exist
if (EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/test AND EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/test/CMakeLists.txt)
    add_subdirectory(test)
endif ()
