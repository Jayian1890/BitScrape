# Types module - no dependencies
file(GLOB_RECURSE TYPES_SOURCES src/*.cpp)
add_library(types STATIC ${TYPES_SOURCES})
target_include_directories(types PUBLIC ${CMAKE_SOURCE_DIR}/include)

