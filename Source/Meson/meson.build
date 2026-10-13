#{
# Source/Meson/meson.build
# Common project meson definitions.
#}

ALL_WARNINGS = [
    '-Wall',
    '-Wextra',
    '-Wpedantic',
    '-Wshadow',
    '-Wnon-virtual-dtor',
    '-Wold-style-cast',
    '-Wcast-align',
    '-Wunused',
    '-Woverloaded-virtual',
    '-Wconversion',
    '-Wsign-conversion',
    '-Wnull-dereference',
    '-Wdouble-promotion',
    '-Wformat=2',
    '-Wimplicit-fallthrough',
    '-Wno-missing-braces',
    '-Werror',
    '-fvisibility=hidden',
    '-fvisibility-inlines-hidden',
    '-MMD',
    '-Wno-uninitialized',
    '-Wno-error=cpp',
]

# glibc emits a noisy #warning when _FORTIFY_SOURCE is active without optimization.
# Use -Og in debug so __OPTIMIZE__ is defined while keeping debug-friendly codegen.
if get_option('optimization') == '0'
    ALL_WARNINGS += '-Og'
endif

FileSystem = import('fs')

GlobalModules = {}
PublicIncludes = {}

SourceRoot = meson.project_source_root() / 'Source'

ModuleDefinitions = []
