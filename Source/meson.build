#{
# Source/meson.build
# Automatically lists and builds all modules.
#}

subdir('Meson/')

ModuleNames = [
    'Core',
    'Maths',
    'RHI',
]

ModuleNames += [
    'Scene',
    'Renderer',
    'AppCore',
    # RHIOpenGL must be built before Launch: Launch's public surface bridge header
    # exposes RHIOpenGL::FOpenGLNativeWindowDesc and depends on that module.
    'RHIOpenGL',
    'Launch'
    #'RHIVulkan',
]

ModuleDefinitions = []

foreach ModuleName : ModuleNames
    if FileSystem.is_file(ModuleName / 'meson.build')
        subdir(ModuleName)
        ModuleDefinitions += [ModuleDefinition + { 'SourceDirRel' : ModuleName }]
    endif
endforeach

subdir('Meson/Public')
subdir('Meson/Private')
subdir('Meson/Tests')
subdir('Meson/Benchmarks')
