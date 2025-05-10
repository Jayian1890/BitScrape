# Only build the CLI application for now
if get_option('build_cli')
    # CLI application that uses the types, event, and beacon modules
    executable(
        'bitscrape_cli',
        'cli/src/main.cpp',
        dependencies: [
            dependency('bitscrape-types'),
            dependency('bitscrape-event'),
            dependency('bitscrape-beacon'),
        ],
        install: true,
    )
endif
