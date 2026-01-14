# Storage module - depends on types and bencode
file(GLOB_RECURSE STORAGE_SOURCES src/*.cpp)
add_library(storage STATIC ${STORAGE_SOURCES})
target_include_directories(storage PUBLIC ${CMAKE_SOURCE_DIR}/include)
target_link_libraries(storage PUBLIC types bencode SQLite::SQLite3)

