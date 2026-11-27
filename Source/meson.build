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

if host_machine.system() == 'linux'
    ModuleNames += [ 'RHIOpenGL' ]
endif

ModuleNames += [
    'Scene',
    'Renderer',
    'AppCore',
    'Launch',
    # TODO: add validators before including Vulkan:
    # if the OS has Vulkan support, include it
    'RHIVulkan',
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
