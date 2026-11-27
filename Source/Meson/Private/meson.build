#{
# Source/Meson/Private/meson.build
# Builds each module's target from its Private/ sources.
#
# Reads  : ModuleDefinitions, PublicIncludes, GlobalModules, ALL_WARNINGS
# Writes : GlobalModules, GlobalTestModules
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
        FindResult = run_command(
            'find', PrivatePath,
            '-type', 'f', '(', '-name', '*.cpp', '-o', '-name', '*.c', ')',
            check : false,
        )
        foreach FilePath : FindResult.stdout().strip().split('\n')
            if FilePath != ''
                PrivateSources += files(FilePath)
            endif
        endforeach
    endif

    ResolvedDependencies = []

    foreach DependencyName : ModuleDependencies
        if not GlobalModules.has_key(DependencyName)
            error(
                'Module @0@ depends on @1@ which is not built yet or not found.'
                .format(ModuleName, DependencyName)
            )
        endif
        ResolvedDependencies += GlobalModules.get(DependencyName)
    endforeach

    ResolvedDependencies += ModuleDefinition.get('ExternalDeps', [])

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
                dependencies : ResolvedDependencies,
                cpp_args : CompileArguments,
                c_args : CompileArguments,
                install : false,
            )

            ModuleTarget = executable(
                ModuleName, MainSources,
                include_directories : IncludeDirectories,
                dependencies : ResolvedDependencies,
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
                dependencies : ResolvedDependencies,
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

        ModuleImplementation = static_library(
            ModuleName + 'Implementation', PrivateSources,
            include_directories : IncludeDirectories,
            dependencies : ResolvedDependencies,
            cpp_args : CompileArguments + BuildDefines,
            c_args : CompileArguments + BuildDefines,
            pic : true,
            install : false,
        )
        ModuleTarget = shared_library(
            ModuleName,
            link_whole : ModuleImplementation,
            include_directories : IncludeDirectories,
            dependencies : ResolvedDependencies,
            cpp_args : CompileArguments + BuildDefines,
            c_args : CompileArguments + BuildDefines,
            install : true,
        )
        ModuleDependency = declare_dependency(
            include_directories : PublicIncludeDirectories,
            link_with : ModuleTarget,
            dependencies : ResolvedDependencies,
        )
        ModuleTestDependency = declare_dependency(
            include_directories : PublicIncludeDirectories,
            link_whole : ModuleImplementation,
            dependencies : ResolvedDependencies,
        )

    elif ModuleType == 'StaticLibrary'
        ModuleTarget = static_library(
            ModuleName, PrivateSources,
            include_directories : IncludeDirectories,
            dependencies : ResolvedDependencies,
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
