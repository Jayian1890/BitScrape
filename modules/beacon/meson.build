# Include directories
inc_dir = include_directories('include')

# Dependencies
types_inc = include_directories('../types/include')
types_lib = dependency('bitscrape-types', fallback: ['types', 'types_dep'])
types_dep = declare_dependency(include_directories: types_inc, dependencies: [types_lib])

event_inc = include_directories('../event/include')
event_lib = dependency('bitscrape-event', fallback: ['event', 'event_dep'])
event_dep = declare_dependency(include_directories: event_inc, dependencies: [event_lib])

# Source files
src_files = [
    'src/beacon_sink.cpp',
    'src/console_sink.cpp',
    'src/file_sink.cpp',
    'src/event_sink.cpp',
    'src/beacon.cpp',
    'src/beacon_adapter.cpp',
]

# Library
beacon_lib = library(
    'bitscrape-beacon',
    src_files,
    include_directories: inc_dir,
    dependencies: [types_dep, event_dep, thread_dep],
    install: true,
)

# Dependency object
beacon_dep = declare_dependency(
    include_directories: inc_dir,
    link_with: beacon_lib,
    dependencies: [types_dep, event_dep, thread_dep],
)

# Make the library available to other modules
meson.override_dependency('bitscrape-beacon', beacon_dep)

# Tests
if get_option('build_tests')
    gtest_dep = dependency('gtest', required: false)
    gtest_main_dep = dependency('gtest_main', required: false)
    if gtest_dep.found() and gtest_main_dep.found()
        test_src = [
            'tests/unit/beacon_sink_test.cpp',
            'tests/unit/console_sink_test.cpp',
            'tests/unit/file_sink_test.cpp',
            'tests/unit/event_sink_test.cpp',
            'tests/unit/beacon_test.cpp',
            'tests/unit/beacon_adapter_test.cpp',
        ]

        test_exe = executable(
            'beacon_test',
            test_src,
            dependencies: [beacon_dep, gtest_dep, gtest_main_dep],
            install: false,
        )

        test('beacon_test', test_exe)
    endif
endif

# Install headers
install_headers(
    'include/bitscrape/beacon/beacon_sink.hpp',
    'include/bitscrape/beacon/console_sink.hpp',
    'include/bitscrape/beacon/file_sink.hpp',
    'include/bitscrape/beacon/event_sink.hpp',
    'include/bitscrape/beacon/beacon.hpp',
    'include/bitscrape/beacon/beacon_adapter.hpp',
    subdir: 'bitscrape/beacon',
)
