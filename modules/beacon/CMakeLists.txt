# Beacon module - depends on types
file(GLOB_RECURSE BEACON_SOURCES src/*.cpp)
add_library(beacon STATIC ${BEACON_SOURCES})
target_include_directories(beacon PUBLIC ${CMAKE_SOURCE_DIR}/include)
target_link_libraries(beacon PUBLIC types)

