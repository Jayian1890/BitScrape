# Include directories
inc_dir = include_directories('include')

# Dependencies
types_inc = include_directories('../types/include')
types_lib = dependency('bitscrape-types', fallback: ['types', 'types_dep'])
types_dep = declare_dependency(include_directories: types_inc, dependencies: [types_lib])

event_inc = include_directories('../event/include')
event_lib = dependency('bitscrape-event', fallback: ['event', 'event_dep'])
event_dep = declare_dependency(include_directories: event_inc, dependencies: [event_lib])

bencode_inc = include_directories('../bencode/include')
bencode_lib = dependency('bitscrape-bencode', fallback: ['bencode', 'bencode_dep'])
bencode_dep = declare_dependency(include_directories: bencode_inc, dependencies: [bencode_lib])

network_inc = include_directories('../network/include')
network_lib = dependency('bitscrape-network', fallback: ['network', 'network_dep'])
network_dep = declare_dependency(include_directories: network_inc, dependencies: [network_lib])

# Source files
src_files = [
    'src/dht_message.cpp',
    'src/dht_message_factory.cpp',
    'src/routing_table.cpp',
    'src/k_bucket.cpp',
    'src/node_lookup.cpp',
    'src/bootstrap.cpp',
    'src/dht_session.cpp',
    'src/token_manager.cpp',
]

# Library
dht_lib = library(
    'bitscrape-dht',
    src_files,
    include_directories: inc_dir,
    dependencies: [types_dep, event_dep, bencode_dep, network_dep, thread_dep],
    install: true,
)

# Dependency object
dht_dep = declare_dependency(
    include_directories: inc_dir,
    link_with: dht_lib,
    dependencies: [types_dep, event_dep, bencode_dep, network_dep, thread_dep],
)

# Tests
if get_option('build_tests')
    gtest_dep = dependency('gtest', required: false)
    gtest_main_dep = dependency('gtest_main', required: false)
    if gtest_dep.found() and gtest_main_dep.found()
        test_src = [
            'tests/unit/dht_message_test.cpp',
            'tests/unit/dht_message_factory_test.cpp',
            'tests/unit/routing_table_test.cpp',
            'tests/unit/k_bucket_test.cpp',
            'tests/unit/node_lookup_test.cpp',
            'tests/unit/bootstrap_test.cpp',
            'tests/unit/dht_session_test.cpp',
            'tests/unit/token_manager_test.cpp',
        ]

        test_exe = executable(
            'dht_test',
            test_src,
            dependencies: [dht_dep, gtest_dep, gtest_main_dep],
            install: false,
        )

        test('dht_test', test_exe)
    endif
endif

# Install headers
install_headers(
    'include/bitscrape/dht/dht_message.hpp',
    'include/bitscrape/dht/dht_message_factory.hpp',
    'include/bitscrape/dht/routing_table.hpp',
    'include/bitscrape/dht/k_bucket.hpp',
    'include/bitscrape/dht/node_lookup.hpp',
    'include/bitscrape/dht/bootstrap.hpp',
    'include/bitscrape/dht/dht_session.hpp',
    'include/bitscrape/dht/token_manager.hpp',
    subdir: 'bitscrape/dht',
)
