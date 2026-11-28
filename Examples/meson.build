#{
# Examples/meson.build
# Builds external examples after the engine modules exist.
#
# Reads  : ALL_WARNINGS, FileSystem, OSName, RuntimeLibraries, GlobalModules, PublicIncludes,
#          PythonInterpreter, SourceGlobber
#}

ExamplesRoot = meson.current_source_dir()
ExampleNames = [ 'Lumina' ]

RuntimeStager = files(meson.project_source_root() / 'Meson' / 'MesonStageRuntimeDlls.py')

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

    if FileSystem.is_dir(ExamplePrivatePath)
        FindResult = run_command(PythonInterpreter, SourceGlobber, ExamplePrivatePath, check : false)
        if FindResult.returncode() != 0
            error(
                'Could not list sources for example @0@: @1@'
                .format(ExampleName, FindResult.stderr().strip())
            )
        endif
        foreach FilePath : FindResult.stdout().strip().split('\n')
            FilePath = FilePath.strip()
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
    ExampleCppArgs = ALL_WARNINGS
    if ExampleName == 'Lumina'
        ExampleCppArgs += '-DLUMINA_SHADER_INSTALL_DIR="' + (get_option('prefix') / LuminaShaderInstallDir) + '"'
    endif

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

    ExampleTarget = executable(
        ExampleName + 'Example', ExampleSources,
        include_directories : ExampleIncludeDirectories,
        dependencies : ExampleDependencies,
        cpp_args : ExampleCppArgs,
        c_args : ALL_WARNINGS,
        install : true,
    )

    # Windows has no RUNPATH: the loader finds DLLs in the executable's own
    # directory, so stage them there instead of relying on a PATH variable.
    if OSName == 'windows' and RuntimeLibraries.length() > 0
        custom_target(
            ExampleName + 'RuntimeDlls',
            input : RuntimeLibraries,
            output : ExampleName + 'Example.runtime.stamp',
            command : [PythonInterpreter, RuntimeStager, '@OUTPUT@', '@INPUT@'],
            build_by_default : true,
            depends : ExampleTarget,
        )
    endif
endforeach
