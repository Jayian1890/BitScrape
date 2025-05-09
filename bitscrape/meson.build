project('bitscrape', 'cpp',
  version : '0.1.0',
  default_options : [
    'cpp_std=c++23',
    'warning_level=3',
    'werror=true',
  ],
)

# Add compiler flags
cpp = meson.get_compiler('cpp')
add_project_arguments(cpp.get_supported_arguments([
  '-Wno-unused-parameter',
  '-Wno-unused-variable',
  '-Wno-missing-field-initializers',
]), language: 'cpp')

# Check for required dependencies
thread_dep = dependency('threads')
fmt_dep = dependency('fmt', version : '>=8.0.0')
curl_dep = dependency('libcurl', version : '>=7.0.0')

# Common include directories
inc_dirs = include_directories('.')

# Enable AddressSanitizer if requested
if get_option('enable_asan')
  add_project_arguments('-fsanitize=address', language: 'cpp')
  add_project_link_arguments('-fsanitize=address', language: 'cpp')
endif

# Build modules
subdir('modules/types')
subdir('modules/utils')
subdir('modules/event_system')
subdir('modules/network')
subdir('modules/bencode')
subdir('modules/dht')
subdir('modules/bittorrent')
subdir('modules/web')
subdir('modules/persistence')
subdir('modules/core')

# Build applications
subdir('apps')

# Build tools if requested
if get_option('build_tools')
  subdir('tools')
endif

# Build tests if requested
if get_option('build_tests')
  subdir('tests')
endif