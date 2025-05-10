# CLI application
executable(
    'bitscrape_cli',
    'src/main.cpp',
    include_directories: [
        '../modules/types/include',
    ],
    dependencies: [
        meson.get_variable('types_dep'),
    ],
    install: true,
)
