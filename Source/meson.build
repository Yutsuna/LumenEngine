#{
# Source/meson.build
# Automatically lists and builds all modules
#}

subdir('Meson/')

GlobalModules = {}
ExcludedDirectories = ['Meson']
ModuleDefinitions = []

ModuleNames = [
    'Core',
    'AppCore',
    'Launch',
]

foreach ModuleName : ModuleNames
    ModulePath = ModuleName
    if FileSystem.is_file(ModulePath / 'meson.build')
        subdir(ModulePath)
        ModuleDefinition = ModuleDefinition + { 'SourceDirRel' : ModuleName }
        ModuleDefinitions += [ModuleDefinition]
    endif
endforeach

subdir('Meson/ModuleBuilder')