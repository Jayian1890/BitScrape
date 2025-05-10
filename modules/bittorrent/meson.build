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

dht_inc = include_directories('../dht/include')
dht_lib = dependency('bitscrape-dht', fallback: ['dht', 'dht_dep'])
dht_dep = declare_dependency(include_directories: dht_inc, dependencies: [dht_lib])

# Source files
src_files = []

# Library
bittorrent_lib = library(
    'bitscrape-bittorrent',
    src_files,
    include_directories: inc_dir,
    dependencies: [types_dep, event_dep, bencode_dep, network_dep, dht_dep, thread_dep],
    install: true,
)

# Dependency object
bittorrent_dep = declare_dependency(
    include_directories: inc_dir,
    link_with: bittorrent_lib,
    dependencies: [types_dep, event_dep, bencode_dep, network_dep, dht_dep, thread_dep],
)

# Tests
if get_option('build_tests')
    gtest_dep = dependency('gtest', required: false)
    gtest_main_dep = dependency('gtest_main', required: false)
    if gtest_dep.found() and gtest_main_dep.found()
        test_src = []

        if test_src.length() > 0
            test_exe = executable(
                'bittorrent_test',
                test_src,
                dependencies: [bittorrent_dep, gtest_dep, gtest_main_dep],
                install: false,
            )

            test('bittorrent_test', test_exe)
        endif
    endif
endif
