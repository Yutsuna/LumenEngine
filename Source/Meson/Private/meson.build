#{
# Source/Meson/Private/meson.build
# Builds each module's target from its Private/ sources.
#
# Reads  : ModuleDefinitions, PublicIncludes, GlobalModules, ALL_WARNINGS, OSName,
#          PythonInterpreter, SourceGlobber,
#          ModuleDefinition.get('Depends'), ModuleDefinition.get('PrivateDepends'),
#          ModuleDefinition.get('TestsExcludeFiles')
# Writes : GlobalModules, GlobalTestModules, RuntimeLibraries
#}

foreach ModuleDefinition : ModuleDefinitions
    ModuleName = ModuleDefinition.get('Name')
    ModuleType = ModuleDefinition.get('Type', 'StaticLibrary')
    ModuleSourceDirRel = ModuleDefinition.get('SourceDirRel')
    ModuleDependencies = ModuleDefinition.get('Depends', [])

    ModulePath = SourceRoot / ModuleSourceDirRel
    PrivatePath = ModulePath / ModuleDefinition.get('Private', 'Private')

    PublicIncludeDirectories = PublicIncludes.get(ModuleName, [])
    IncludeDirectories = PublicIncludeDirectories

    PrivateDirectory = ModuleDefinition.get('Private', 'Private')
    RelPrivatePath = '../../@0@/@1@'.format(ModuleSourceDirRel, PrivateDirectory)

    if FileSystem.is_dir(RelPrivatePath)
        IncludeDirectories += include_directories(RelPrivatePath)
    endif

    PrivateSources = []

    if FileSystem.is_dir(PrivatePath)
        FindResult = run_command(PythonInterpreter, SourceGlobber, PrivatePath, check : false)
        if FindResult.returncode() != 0
            error(
                'Could not list sources for module @0@: @1@'
                .format(ModuleName, FindResult.stderr().strip())
            )
        endif
        foreach FilePath : FindResult.stdout().strip().split('\n')
            FilePath = FilePath.strip()
            if FilePath != ''
                PrivateSources += files(FilePath)
            endif
        endforeach
    endif

    TestsExcludeFiles = ModuleDefinition.get('TestsExcludeFiles', [])
    HasTestExclusions = TestsExcludeFiles.length() > 0
    ExcludedPrivateFiles = []
    TestPrivateSources = PrivateSources

    if HasTestExclusions and ModuleType != 'SharedLibrary'
        error(
            'Module @0@: TestsExcludeFiles is only supported by SharedLibrary modules.'
            .format(ModuleName)
        )
    endif

    if HasTestExclusions
        foreach ExcludedFileName : TestsExcludeFiles
            ExcludedFilePath = PrivatePath / ExcludedFileName
            if not FileSystem.is_file(ExcludedFilePath)
                error(
                    'Module @0@ tests exclude @1@, which does not exist.'
                    .format(ModuleName, ExcludedFilePath)
                )
            endif
            ExcludedPrivateFiles += files(ExcludedFilePath)
        endforeach

        TestPrivateSources = []
        foreach SourcePath : PrivateSources
            if not ExcludedPrivateFiles.contains(SourcePath)
                TestPrivateSources += SourcePath
            endif
        endforeach

        if TestPrivateSources.length() == 0
            error(
                'Module @0@: TestsExcludeFiles removes every private source.'
                .format(ModuleName)
            )
        endif
    endif

    ResolvedDependencies = []
    ResolvedPrivateDependencies = []

    foreach DependencyName : ModuleDependencies
        if not GlobalModules.has_key(DependencyName)
            error(
                'Module @0@ depends on @1@ which is not built yet or not found.'
                .format(ModuleName, DependencyName)
            )
        endif
        ResolvedDependencies += GlobalModules.get(DependencyName)
    endforeach

    # Private dependencies are compiled and linked into this module's target but are
    # deliberately absent from the dependency exported to consumers.
    foreach DependencyName : ModuleDefinition.get('PrivateDepends', [])
        if not GlobalModules.has_key(DependencyName)
            error(
                'Module @0@ privately depends on @1@ which is not built yet or not found.'
                .format(ModuleName, DependencyName)
            )
        endif
        ResolvedPrivateDependencies += GlobalModules.get(DependencyName)
    endforeach

    TargetDependencies = ResolvedDependencies + ResolvedPrivateDependencies

    ResolvedDependencies += ModuleDefinition.get('ExternalDeps', [])
    TargetDependencies += ModuleDefinition.get('ExternalDeps', [])

    CompileArguments = ALL_WARNINGS

    if ModuleType == 'Executable'
        MainFilePath = PrivatePath / 'Main.cpp'
        MainSources = []
        LibrarySources = PrivateSources

        MainFile = files(MainFilePath)[0]
        foreach SourcePath : PrivateSources
            if SourcePath == MainFile
                MainSources += SourcePath
                LibrarySources = []
                foreach OtherPath : PrivateSources
                    if OtherPath != SourcePath
                        LibrarySources += OtherPath
                    endif
                endforeach
            endif
        endforeach

        if MainSources.length() == 0
            MainSources = PrivateSources
            LibrarySources = []
        endif

        if LibrarySources.length() > 0
            ModuleLibrary = static_library(
                ModuleName + 'Lib', LibrarySources,
                include_directories : IncludeDirectories,
                dependencies : TargetDependencies,
                cpp_args : CompileArguments,
                c_args : CompileArguments,
                install : false,
            )

            ModuleTarget = executable(
                ModuleName, MainSources,
                include_directories : IncludeDirectories,
                dependencies : TargetDependencies,
                link_with : ModuleLibrary,
                cpp_args : CompileArguments,
                c_args : CompileArguments,
                install : true,
            )
            ModuleDependency = declare_dependency(
                include_directories : PublicIncludeDirectories,
                link_with : ModuleLibrary,
                dependencies : ResolvedDependencies,
            )
        else
            ModuleTarget = executable(
                ModuleName, MainSources,
                include_directories : IncludeDirectories,
                dependencies : TargetDependencies,
                cpp_args : CompileArguments,
                c_args : CompileArguments,
                install : true,
            )
            ModuleDependency = declare_dependency(
                include_directories : PublicIncludeDirectories,
                dependencies : ResolvedDependencies,
            )
        endif

    elif ModuleType == 'SharedLibrary'
        ExportConfiguration = configuration_data()
        ExportConfiguration.set('MODULE', ModuleName.to_upper())

        configure_file(
            input : meson.project_source_root() / 'Meson' / 'ExportTemplate.hpp.inl',
            output : '@0@Export.hpp'.format(ModuleName),
            configuration : ExportConfiguration,
        )

        ExportIncludeDirectories = include_directories('.')
        IncludeDirectories += ExportIncludeDirectories
        PublicIncludeDirectories += ExportIncludeDirectories
        BuildDefines = ['-D' + ModuleName.to_upper() + '_BUILD_SHARED']

        # On Windows the loader looks in the executable's directory first. Installing
        # the DLLs next to the executables removes any PATH requirement.
        SharedLibraryInstallDir = OSName == 'windows' ? get_option('bindir') : []

        ModuleImplementation = static_library(
            ModuleName + 'Implementation', PrivateSources,
            include_directories : IncludeDirectories,
            dependencies : TargetDependencies,
            cpp_args : CompileArguments + BuildDefines,
            c_args : CompileArguments + BuildDefines,
            pic : true,
            install : false,
        )
        ModuleTarget = shared_library(
            ModuleName,
            link_whole : ModuleImplementation,
            include_directories : IncludeDirectories,
            dependencies : TargetDependencies,
            cpp_args : CompileArguments + BuildDefines,
            c_args : CompileArguments + BuildDefines,
            install : true,
            install_dir : SharedLibraryInstallDir,
        )

        if OSName == 'windows'
            RuntimeLibraries += [ModuleTarget]
        endif

        ModuleDependency = declare_dependency(
            include_directories : PublicIncludeDirectories,
            link_with : ModuleTarget,
            dependencies : ResolvedDependencies,
        )
        ModuleTestDependency = declare_dependency(
            include_directories : PublicIncludeDirectories,
            link_whole : ModuleImplementation,
            dependencies : TargetDependencies,
        )

        if HasTestExclusions
            ModuleTestImplementation = static_library(
                ModuleName + 'TestImplementation', TestPrivateSources,
                include_directories : IncludeDirectories,
                dependencies : TargetDependencies,
                cpp_args : CompileArguments + BuildDefines,
                c_args : CompileArguments + BuildDefines,
                pic : true,
                install : false,
            )
            ModuleTestDependency = declare_dependency(
                include_directories : PublicIncludeDirectories,
                link_whole : ModuleTestImplementation,
                dependencies : TargetDependencies,
            )
        endif

    elif ModuleType == 'StaticLibrary'
        ModuleTarget = static_library(
            ModuleName, PrivateSources,
            include_directories : IncludeDirectories,
            dependencies : TargetDependencies,
            cpp_args : CompileArguments,
            c_args : CompileArguments,
            install : true,
        )
        ModuleDependency = declare_dependency(
            include_directories : PublicIncludeDirectories,
            link_with : ModuleTarget,
            dependencies : ResolvedDependencies,
        )

    else
        error('Unknown module type: @0@'.format(ModuleType))
    endif

    if ModuleType != 'SharedLibrary'
        ModuleTestDependency = ModuleDependency
    endif

    GlobalModules += { ModuleName : ModuleDependency }
    GlobalTestModules += { ModuleName : ModuleTestDependency }
endforeach
