#{
# meson.build
# Meson entry-point.
#}

project(
    'Lumen',
    'cpp',
    'c',
    version : run_command('cat', 'VERSION.md', check : true).stdout().strip(),
    default_options : [
        'cpp_std=c++26',
        'c_std=c23',
        'default_library=shared',
        'warning_level=0',
    ],
    meson_version : '>=1.10',
)

add_project_arguments('-freflection', language : 'cpp')

subdir('Source/')
