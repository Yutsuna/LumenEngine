#{
# Examples/meson.build
# Builds external examples after the engine modules exist.
#
# Reads  : ALL_WARNINGS, FileSystem, GlobalModules, PublicIncludes
#}

ExamplesRoot = meson.current_source_dir()
ExampleNames = [ 'Lumina' ]

foreach ExampleName : ExampleNames
    ExampleDirRel = ExampleName
    ExampleDir = ExamplesRoot / ExampleName

    if not FileSystem.is_file(ExampleDirRel / 'meson.build')
        error('Example @0@ has no meson.build'.format(ExampleName))
    endif

    subdir(ExampleDirRel)

    ExampleDefinition = ModuleDefinition
    ExamplePrivateDirectory = ExampleDefinition.get('Private', 'Private')
    ExamplePublicDirectory = ExampleDefinition.get('Public', 'Public')
    ExamplePrivatePath = ExampleDir / ExamplePrivateDirectory

    ExampleSources = []

    if FileSystem.is_dir(ExampleDirRel / ExamplePrivateDirectory)
        FindResult = run_command(
            'find', ExamplePrivatePath,
            '-type', 'f', '(', '-name', '*.cpp', '-o', '-name', '*.c', ')',
            check : false,
        )
        foreach FilePath : FindResult.stdout().strip().split('\n')
            if FilePath != ''
                ExampleSources += files(FilePath)
            endif
        endforeach
    endif

    ExampleIncludeDirectories = [
        include_directories(ExampleDirRel / ExamplePublicDirectory),
        include_directories(ExampleDirRel / ExamplePrivateDirectory),
    ]
    ExampleDependencies = []

    foreach DependencyName : ExampleDefinition.get('Depends', [])
        if not GlobalModules.has_key(DependencyName)
            error(
                'Example @0@ depends on @1@ which is not built yet or not found.'
                .format(ExampleName, DependencyName)
            )
        endif
        ExampleDependencies += GlobalModules.get(DependencyName)
        ExampleIncludeDirectories += PublicIncludes.get(DependencyName, [])
    endforeach

    executable(
        ExampleName + 'Example', ExampleSources,
        include_directories : ExampleIncludeDirectories,
        dependencies : ExampleDependencies,
        cpp_args : ALL_WARNINGS,
        c_args : ALL_WARNINGS,
        install : true,
    )
endforeach
