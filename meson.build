project('librw', 'cpp',
    version: '2.0.0',
    default_options: [
        'cpp_std=c++17',
        'b_lto=true',
        'b_ndebug=if-release'
    ]
)

version_file = configure_file(
    input: 'version.cpp.in',
    output: 'version.cpp',
    configuration: {
        'version': meson.project_version()
    }
)

inc = include_directories('include')

librw = static_library(
    'rw', [
        'argparse.cpp',
        'fmt/format.cc',
        'logging.cpp',
        'profiling.cpp',
        'utf8.cpp',
        version_file
    ],
    include_directories : inc,
    install : true
)

librw_dep = declare_dependency(
    include_directories : inc,
    link_with : librw
)

executable(
    'librw-bench', [
        'bench/main.cpp',
        'bench/map.cpp',
        'bench/utf8.cpp',
        'test/utf8-data.cpp',
    ],
    dependencies: [librw_dep],
    include_directories : inc
)

testexe = executable(
    'librw-test', [
        'test/argparse.cpp',
        'test/main.cpp',
        'test/map.cpp',
        'test/utf8.cpp',
        'test/utf8-data.cpp',
    ],
    dependencies: [librw_dep],
    include_directories : inc
)

test('librw test', testexe)

