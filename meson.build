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

# MinGW-w64 keeps WaitOnAddress/WakeByAddress* in libsynchronization, and GCC 14+
# keeps std::print's terminal helpers in libstdc++exp. Both are implicit on MSVC/Linux.
if host_machine.system() == 'windows'
    add_project_link_arguments('-lsynchronization', '-lstdc++exp', language : 'cpp')
endif

subdir('Source')
subdir('Examples')
