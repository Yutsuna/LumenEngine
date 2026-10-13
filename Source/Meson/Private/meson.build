#{
# Source/Meson/Private/meson.build
# Builds each module's target from its Private/ sources.
#
# Reads  : ModuleDefinitions, PublicIncludes, GlobalModules, ALL_WARNINGS
# Writes : GlobalModules
#}

foreach ModuleDefinition : ModuleDefinitions
    ModuleName = ModuleDefinition.get('Name')
    ModuleType = ModuleDefinition.get('Type', 'StaticLibrary')
    ModuleSourceDirRel = ModuleDefinition.get('SourceDirRel')
    ModuleDependencies = ModuleDefinition.get('Depends', [])

    ModulePath = SourceRoot / ModuleSourceDirRel
    PrivatePath = ModulePath / ModuleDefinition.get('Private', 'Private')

    IncludeDirectories = PublicIncludes.get(ModuleName, [])

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

    CompileArguments = ALL_WARNINGS

    if ModuleType == 'Executable'
        ModuleTarget = executable(
            ModuleName, PrivateSources,
            include_directories : IncludeDirectories,
            dependencies : ResolvedDependencies,
            cpp_args : CompileArguments,
            c_args : CompileArguments,
            install : true,
        )
        ModuleDependency = declare_dependency(
            include_directories : IncludeDirectories,
            dependencies : ResolvedDependencies,
        )

    elif ModuleType == 'SharedLibrary'
        ExportConfiguration = configuration_data()
        ExportConfiguration.set('MODULE', ModuleName.to_upper())

        configure_file(
            input : meson.project_source_root() / 'Meson' / 'ExportTemplate.hpp.inl',
            output : '@0@Export.hpp'.format(ModuleName),
            configuration : ExportConfiguration,
        )

        IncludeDirectories += include_directories('.')
        BuildDefines = ['-D' + ModuleName.to_upper() + '_BUILD_SHARED']

        ModuleTarget = shared_library(
            ModuleName, PrivateSources,
            include_directories : IncludeDirectories,
            dependencies : ResolvedDependencies,
            cpp_args : CompileArguments + BuildDefines,
            c_args : CompileArguments + BuildDefines,
            install : true,
        )
        ModuleDependency = declare_dependency(
            include_directories : IncludeDirectories,
            link_with : ModuleTarget,
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
            include_directories : IncludeDirectories,
            link_with : ModuleTarget,
            dependencies : ResolvedDependencies,
        )

    else
        error('Unknown module type: @0@'.format(ModuleType))
    endif

    GlobalModules += { ModuleName : ModuleDependency }
endforeach