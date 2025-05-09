# Include directories
inc_dir = include_directories('include')

# Dependencies
types_inc = include_directories('../types/include')
types_lib = meson.get_compiler('cpp').find_library('bitscrape-types', dirs: meson.current_build_dir() + '/../types')
types_dep = declare_dependency(include_directories: types_inc, dependencies: [types_lib])

# Source files
src_files = [
    'src/event_bus.cpp',
    'src/event_processor.cpp',
    'src/event_adapter.cpp',
    'src/event_filter.cpp',
    'src/async_event_processor.cpp',
]

# Library
event_lib = library(
    'bitscrape-event',
    src_files,
    include_directories: inc_dir,
    dependencies: [types_dep],
    install: true,
)

# Dependency object
event_dep = declare_dependency(
    include_directories: inc_dir,
    link_with: event_lib,
    dependencies: [types_dep],
)

# Tests
if get_option('build_tests')
    gtest_dep = dependency('gtest', required: false)
    gtest_main_dep = dependency('gtest_main', required: false)
    if gtest_dep.found() and gtest_main_dep.found()
        test_src = [
            'tests/unit/event_bus_test.cpp',
            'tests/unit/event_processor_test.cpp',
            'tests/unit/event_adapter_test.cpp',
            'tests/unit/event_filter_test.cpp',
            'tests/unit/async_event_processor_test.cpp',
        ]

        test_exe = executable(
            'event_test',
            test_src,
            dependencies: [event_dep, gtest_dep, gtest_main_dep],
            install: false,
        )

        test('event_test', test_exe)
    endif
endif

# Install headers
install_headers(
    'include/bitscrape/event/event_bus.hpp',
    'include/bitscrape/event/event_processor.hpp',
    'include/bitscrape/event/event_adapter.hpp',
    'include/bitscrape/event/event_filter.hpp',
    'include/bitscrape/event/async_event_processor.hpp',
    subdir: 'bitscrape/event',
)
