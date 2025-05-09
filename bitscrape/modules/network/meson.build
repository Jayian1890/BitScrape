# Include directories
inc_dir = include_directories('include')

# Dependencies
types_inc = include_directories('../types/include')
types_lib = meson.get_compiler('cpp').find_library('bitscrape-types', dirs: meson.current_build_dir() + '/../types')
types_dep = declare_dependency(include_directories: types_inc, dependencies: [types_lib])

# Source files
src_files = [
    'src/socket.cpp',
    'src/address.cpp',
    'src/buffer.cpp',
    'src/udp_socket.cpp',
    'src/tcp_socket.cpp',
    'src/tcp_listener.cpp',
    'src/http_client.cpp',
    'src/nat_traversal.cpp',
]

# Library
network_lib = library(
    'bitscrape-network',
    src_files,
    include_directories: inc_dir,
    dependencies: [types_dep, thread_dep],
    install: true,
)

# Dependency object
network_dep = declare_dependency(
    include_directories: inc_dir,
    link_with: network_lib,
    dependencies: [types_dep, thread_dep],
)

# Tests
if get_option('build_tests')
    gtest_dep = dependency('gtest', required: false)
    gtest_main_dep = dependency('gtest_main', required: false)
    if gtest_dep.found() and gtest_main_dep.found()
        test_src = [
            'tests/unit/socket_test.cpp',
            'tests/unit/address_test.cpp',
            'tests/unit/buffer_test.cpp',
        ]

        test_exe = executable(
            'network_test',
            test_src,
            dependencies: [network_dep, gtest_dep, gtest_main_dep],
            install: false,
        )

        test('network_test', test_exe)
    endif
endif

# Install headers
install_headers(
    'include/bitscrape/network/socket.hpp',
    'include/bitscrape/network/address.hpp',
    'include/bitscrape/network/buffer.hpp',
    'include/bitscrape/network/udp_socket.hpp',
    'include/bitscrape/network/tcp_socket.hpp',
    'include/bitscrape/network/tcp_listener.hpp',
    'include/bitscrape/network/http_client.hpp',
    'include/bitscrape/network/nat_traversal.hpp',
    subdir: 'bitscrape/network',
)
