# Event module - depends on types
file(GLOB_RECURSE EVENT_SOURCES src/*.cpp)
add_library(event STATIC ${EVENT_SOURCES})
target_include_directories(event PUBLIC ${CMAKE_SOURCE_DIR}/include)
target_link_libraries(event PUBLIC types)

