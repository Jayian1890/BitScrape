# Include directories
inc_dir = include_directories('include')

# Dependencies
types_inc = include_directories('../types/include')
types_lib = meson.get_compiler('cpp').find_library('bitscrape-types', dirs: meson.current_build_dir() + '/../types')
types_dep = declare_dependency(include_directories: types_inc, dependencies: [types_lib])

event_inc = include_directories('../event/include')
event_lib = meson.get_compiler('cpp').find_library('bitscrape-event', dirs: meson.current_build_dir() + '/../event')
event_dep = declare_dependency(include_directories: event_inc, dependencies: [event_lib])

# Source files
src_files = [
    'src/bencode_value.cpp',
    'src/bencode_encoder.cpp',
    'src/bencode_decoder.cpp',
    'src/bencode_stream.cpp',
    'src/bencode_event_processor.cpp',
]

# Library
bencode_lib = library(
    'bitscrape-bencode',
    src_files,
    include_directories: inc_dir,
    dependencies: [types_dep, event_dep],
    install: true,
)

# Dependency object
bencode_dep = declare_dependency(
    include_directories: inc_dir,
    link_with: bencode_lib,
    dependencies: [types_dep, event_dep],
)

# Tests
if get_option('build_tests')
    gtest_dep = dependency('gtest', required: false)
    gtest_main_dep = dependency('gtest_main', required: false)
    if gtest_dep.found() and gtest_main_dep.found()
        test_src = [
            'tests/unit/bencode_value_test.cpp',
            'tests/unit/bencode_encoder_test.cpp',
            'tests/unit/bencode_decoder_test.cpp',
            'tests/unit/bencode_stream_test.cpp',
            'tests/unit/bencode_event_processor_test.cpp',
        ]

        test_exe = executable(
            'bencode_test',
            test_src,
            dependencies: [bencode_dep, gtest_dep, gtest_main_dep],
            install: false,
        )

        test('bencode_test', test_exe)
    endif
endif

# Install headers
install_headers(
    'include/bitscrape/bencode/bencode_value.hpp',
    'include/bitscrape/bencode/bencode_encoder.hpp',
    'include/bitscrape/bencode/bencode_decoder.hpp',
    'include/bitscrape/bencode/bencode_stream.hpp',
    'include/bitscrape/bencode/bencode_event_processor.hpp',
    subdir: 'bitscrape/bencode',
)
