# Bencode module - depends on types
file(GLOB_RECURSE BENCODE_SOURCES src/*.cpp)
add_library(bencode STATIC ${BENCODE_SOURCES})
target_include_directories(bencode PUBLIC ${CMAKE_SOURCE_DIR}/include)
target_link_libraries(bencode PUBLIC types)

