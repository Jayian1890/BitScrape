# Only build the CLI application for now
if get_option('build_cli')
    # Simple CLI application that uses the types module
    executable(
        'bitscrape_cli',
        'cli/src/main.cpp',
        dependencies: [
            dependency('bitscrape-types'),
        ],
        install: true,
    )
endif
