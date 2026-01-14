# Web UI library - depends on types, network
file(GLOB_RECURSE WEB_SOURCES *.cpp)
add_library(web STATIC ${WEB_SOURCES})
target_include_directories(web PUBLIC ${CMAKE_SOURCE_DIR}/include)
target_link_libraries(web PUBLIC types network)

