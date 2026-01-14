# Network module - depends on types
file(GLOB_RECURSE NETWORK_SOURCES src/*.cpp)
add_library(network STATIC ${NETWORK_SOURCES})
target_include_directories(network PUBLIC ${CMAKE_SOURCE_DIR}/include)
target_link_libraries(network PUBLIC types)

