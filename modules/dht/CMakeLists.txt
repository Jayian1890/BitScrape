# DHT module - depends on types, bencode, network, beacon, lock
file(GLOB_RECURSE DHT_SOURCES src/*.cpp)
add_library(dht STATIC ${DHT_SOURCES})
target_include_directories(dht PUBLIC ${CMAKE_SOURCE_DIR}/include)
target_link_libraries(dht PUBLIC types bencode network beacon lock)

