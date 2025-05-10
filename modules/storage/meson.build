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
src_files = []

# Library
storage_lib = library(
    'bitscrape-storage',
    src_files,
    include_directories: inc_dir,
    dependencies: [types_dep, event_dep, thread_dep],
    install: true,
)

# Dependency object
storage_dep = declare_dependency(
    include_directories: inc_dir,
    link_with: storage_lib,
    dependencies: [types_dep, event_dep, thread_dep],
)

# Tests
if get_option('build_tests')
    gtest_dep = dependency('gtest', required: false)
    gtest_main_dep = dependency('gtest_main', required: false)
    if gtest_dep.found() and gtest_main_dep.found()
        test_src = []

        if test_src.length() > 0
            test_exe = executable(
                'storage_test',
                test_src,
                dependencies: [storage_dep, gtest_dep, gtest_main_dep],
                install: false,
            )

            test('storage_test', test_exe)
        endif
    endif
endif
