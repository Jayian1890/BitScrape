project(
    'bitscrape',
    'cpp',
    version: '0.1.0',
    default_options: [
        'cpp_std=c++23',
        'warning_level=3',
        'werror=false',
    ],
)

# Add compiler flags
cpp = meson.get_compiler('cpp')
add_project_arguments(
    cpp.get_supported_arguments(
        [
            '-Wno-unused-parameter',
            '-Wno-unused-variable',
            '-Wno-missing-field-initializers',
            '-D_LIBCPP_HARDENING_MODE=_LIBCPP_HARDENING_MODE_FAST',
        ],
    ),
    language: 'cpp',
)

# Check for required dependencies
thread_dep = dependency('threads')

# Common include directories
inc_dirs = include_directories(
    '.',
    'modules/types/include',
    'modules/event/include',
    'modules/beacon/include'
)

# Enable AddressSanitizer if requested
if get_option('enable_asan')
    add_project_arguments('-fsanitize=address', language: 'cpp')
    add_project_link_arguments('-fsanitize=address', language: 'cpp')
endif

# Build modules
subdir('modules/types')
subdir('modules/event')
subdir('modules/beacon')
subdir('modules/bencode')
subdir('modules/network')

# Build applications
subdir('apps')

# Build tests if requested
if get_option('build_tests')
    # subdir('tests')
endif
