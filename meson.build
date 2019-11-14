project('librw', 'cpp',
    version: '1.0.0',
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
        'fmt/format.cc',
        'fmt/posix.cc',
        'logging.cpp',
        version_file
    ],
    include_directories : inc,
    install : true
)

librw_dep = declare_dependency(
    include_directories : inc,
    link_with : librw
)
