#{
# Source/meson.build
# Automatically lists and builds all modules.
#}

subdir('Meson/')

ModuleNames = [
    'Core',
    'Maths',
    'RHI',
    # Backends are built before Renderer: Renderer links them as private dependencies
    # and selects one at startup. No backend is propagated to Renderer's consumers.
    'RHIOpenGL',
]

ModuleNames += [
    'Scene',
    'Renderer',
    'AppCore',
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
