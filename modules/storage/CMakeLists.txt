# Storage module - depends on types and bencode
file(GLOB_RECURSE STORAGE_SOURCES src/*.cpp)
add_library(storage STATIC ${STORAGE_SOURCES})
target_include_directories(storage PUBLIC ${CMAKE_SOURCE_DIR}/include)
find_package(SQLite3 REQUIRED)

# If the imported target SQLite::SQLite3 is not provided by the platform
# create a simple IMPORTED target that points to the found library.
if (NOT TARGET SQLite::SQLite3)
	if (TARGET SQLITE3::SQLITE3)
		# Some environments provide a different target name
		add_library(SQLite::SQLite3 ALIAS SQLITE3::SQLITE3)
	else()
		find_library(_SQLITE3_LIB sqlite3)
		if (_SQLITE3_LIB)
			add_library(SQLite::SQLite3 UNKNOWN IMPORTED)
			set_target_properties(SQLite::SQLite3 PROPERTIES IMPORTED_LOCATION "${_SQLITE3_LIB}")
		endif()
	endif()
endif()

target_link_libraries(storage PUBLIC types bencode SQLite::SQLite3)

