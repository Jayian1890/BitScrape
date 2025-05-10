# Include directories
inc_dir = include_directories('include')

# Source files
src_files = [
    'src/node_id.cpp',
    'src/info_hash.cpp',
    'src/endpoint.cpp',
    'src/event_types.cpp',
    'src/message_types.cpp',
    'src/dht_node.cpp',
    'src/dht_token.cpp',
    'src/dht_routing_table_entry.cpp',
    'src/metadata_info.cpp',
    'src/metadata_piece.cpp',
    'src/torrent_info.cpp',
    'src/beacon_types.cpp',
]

# Library
types_lib = library('bitscrape-types', src_files, include_directories: inc_dir, install: true)

# Dependency object
types_dep = declare_dependency(include_directories: inc_dir, link_with: types_lib)

# Make the library available to other modules
meson.override_dependency('bitscrape-types', types_dep)

# Tests
if get_option('build_tests')
    gtest_dep = dependency('gtest', required: false)
    gtest_main_dep = dependency('gtest_main', required: false)
    if gtest_dep.found() and gtest_main_dep.found()
        test_src = [
            'tests/unit/node_id_test.cpp',
            'tests/unit/info_hash_test.cpp',
            'tests/unit/endpoint_test.cpp',
            'tests/unit/event_types_test.cpp',
            'tests/unit/message_types_test.cpp',
            'tests/unit/dht_node_test.cpp',
            'tests/unit/dht_token_test.cpp',
            'tests/unit/dht_routing_table_entry_test.cpp',
            'tests/unit/metadata_info_test.cpp',
            'tests/unit/metadata_piece_test.cpp',
            'tests/unit/torrent_info_test.cpp',
            'tests/unit/beacon_types_test.cpp',
        ]

        test_exe = executable(
            'types_test',
            test_src,
            dependencies: [types_dep, gtest_dep, gtest_main_dep],
            install: false,
        )

        test('types_test', test_exe)
    endif
endif

# Install headers
install_headers(
    'include/bitscrape/types/node_id.hpp',
    'include/bitscrape/types/info_hash.hpp',
    'include/bitscrape/types/endpoint.hpp',
    'include/bitscrape/types/event_types.hpp',
    'include/bitscrape/types/message_types.hpp',
    'include/bitscrape/types/dht_node.hpp',
    'include/bitscrape/types/dht_token.hpp',
    'include/bitscrape/types/dht_routing_table_entry.hpp',
    'include/bitscrape/types/metadata_info.hpp',
    'include/bitscrape/types/metadata_piece.hpp',
    'include/bitscrape/types/torrent_info.hpp',
    'include/bitscrape/types/beacon_types.hpp',
    subdir: 'bitscrape/types',
)
