# Lock module - depends on types and beacon
file(GLOB_RECURSE LOCK_SOURCES src/*.cpp)
add_library(lock STATIC ${LOCK_SOURCES})
target_include_directories(lock PUBLIC ${CMAKE_SOURCE_DIR}/include)
target_link_libraries(lock PUBLIC types beacon)

