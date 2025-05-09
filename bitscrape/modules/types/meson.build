project('bitscrape-types', 'cpp',
  version : '0.1.0',
  default_options : ['warning_level=3', 'cpp_std=c++23']
)

# Include directories
inc_dir = include_directories('include')

# Source files
src_files = [
  'src/node_id.cpp',
  'src/info_hash.cpp',
  'src/endpoint.cpp',
  'src/event_types.cpp',
  'src/message_types.cpp',
]

# Library
types_lib = library('bitscrape-types',
  src_files,
  include_directories : inc_dir,
  install : true
)

# Dependency object
types_dep = declare_dependency(
  include_directories : inc_dir,
  link_with : types_lib
)

# Tests
if get_option('build_tests')
  catch2_dep = dependency('catch2', required : false)
  if catch2_dep.found()
    test_src = [
      'tests/unit/node_id_test.cpp',
      'tests/unit/info_hash_test.cpp',
      'tests/unit/endpoint_test.cpp',
      'tests/unit/event_types_test.cpp',
      'tests/unit/message_types_test.cpp',
    ]
    
    test_exe = executable('types_test',
      test_src,
      dependencies : [types_dep, catch2_dep],
      install : false
    )
    
    test('types_test', test_exe)
  endif
endif

# Install headers
install_headers('include/bitscrape/types/node_id.hpp',
                'include/bitscrape/types/info_hash.hpp',
                'include/bitscrape/types/endpoint.hpp',
                'include/bitscrape/types/event_types.hpp',
                'include/bitscrape/types/message_types.hpp',
                subdir : 'bitscrape/types')
