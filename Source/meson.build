#{
# Source/meson.build
# Automatically lists and builds all modules.
#}

subdir('Meson/')

ModuleNames = [
    'Core',
    'AppCore',
    'Launch',
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