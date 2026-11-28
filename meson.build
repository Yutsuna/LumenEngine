#{
# meson.build
# Meson entry-point.
#}

project(
    'Lumen',
    'cpp',
    'c',
    default_options : [
        'cpp_std=c++26',
        'c_std=c23',
        'default_library=shared',
        'warning_level=0',
        'b_ndebug=if-release',
    ],
    meson_version : '>=1.10',
)

add_project_arguments('-freflection', language : 'cpp')

subdir('Source')
subdir('Examples')
