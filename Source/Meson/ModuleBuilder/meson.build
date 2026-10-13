#{
# Meson/ModuleBuilder/meson.build
# Functions definitions for building modules.
#}

FileSystem = import('fs')

foreach ModuleDefinition : ModuleDefinitions
    ModuleName = ModuleDefinition.get('Name')
    ModuleType = ModuleDefinition.get('Type', 'StaticLibrary')
    ModuleSourceDirRel = ModuleDefinition.get('SourceDirRel')
    PrivateDirectory = ModuleDefinition.get('Private', 'Private')
    PublicDirectory = ModuleDefinition.get('Public', 'Public')
    TestsDirectory = ModuleDefinition.get('Tests', 'Tests')
    ModuleDependencies = ModuleDefinition.get('Depends', [])

    PrivateSources = []
    RelPrivatePath = '../../@0@/@1@'.format(ModuleSourceDirRel, PrivateDirectory)

    if FileSystem.is_dir(RelPrivatePath)
        FindResult = run_command(
            'find', RelPrivatePath, '-type', 'f', '(', '-name', '*.cpp', '-o', '-name', '*.c', ')',
            check : false
        )
        RawPaths = FindResult.stdout().strip()
        if RawPaths != ''
            foreach FilePath : RawPaths.split('\n')
                if FilePath != ''
                    PrivateSources += files(FilePath)
                endif
            endforeach
        endif
    endif

    IncludeDirectories = []
    RelPublicPath = '../../@0@/@1@'.format(ModuleSourceDirRel, PublicDirectory)
    if FileSystem.is_dir(RelPublicPath)
        IncludeDirectories += include_directories(RelPublicPath)
    endif

    ResolvedDependencies = []
    foreach DependencyName : ModuleDependencies
        if GlobalModules.has_key(DependencyName)
            ResolvedDependencies += GlobalModules.get(DependencyName)
        else
            error('Module @0@ depends on @1@ which is not built yet or not found.'.format(ModuleName, DependencyName))
        endif
    endforeach

    CompileArguments = ALL_WARNINGS
    ModuleTarget = []
    ModuleDependency = []

    if ModuleType == 'Executable'
        ModuleTarget = executable(
            ModuleName, PrivateSources,
            include_directories : IncludeDirectories,
            dependencies : ResolvedDependencies,
            cpp_args : CompileArguments, c_args : CompileArguments,
            install : true
        )
        ModuleDependency = declare_dependency(
            include_directories : IncludeDirectories,
            dependencies : ResolvedDependencies
        )
    elif ModuleType == 'SharedLibrary'
        ExportConfiguration = configuration_data()
        ExportConfiguration.set('MODULE', ModuleName.to_upper())

        configure_file(
            input : meson.project_source_root() / 'Meson' / 'ExportTemplate.hpp.inl',
            output : '@0@Export.hpp'.format(ModuleName),
            configuration : ExportConfiguration
        )

        IncludeDirectories += include_directories('.')
        BuildDefines = ['-D' + ModuleName.to_upper() + '_BUILD_SHARED']

        ModuleTarget = shared_library(
            ModuleName, PrivateSources,
            include_directories : IncludeDirectories,
            dependencies : ResolvedDependencies,
            cpp_args : CompileArguments + BuildDefines,
            c_args : CompileArguments + BuildDefines,
            install : true
        )
        ModuleDependency = declare_dependency(
            include_directories : IncludeDirectories,
            link_with : ModuleTarget,
            dependencies : ResolvedDependencies
        )
    elif ModuleType == 'StaticLibrary'
        ModuleTarget = static_library(
            ModuleName, PrivateSources,
            include_directories : IncludeDirectories,
            dependencies : ResolvedDependencies,
            cpp_args : CompileArguments, c_args : CompileArguments,
            install : true
        )
        ModuleDependency = declare_dependency(
            include_directories : IncludeDirectories,
            link_with : ModuleTarget,
            dependencies : ResolvedDependencies
        )
    else
        error('Unknown module type: @0@'.format(ModuleType))
    endif

    GlobalModules += { ModuleName : ModuleDependency }

    # Discover and build test suite if enabled
    TestSources = []
    RelTestsPath = '../../@0@/@1@'.format(ModuleSourceDirRel, TestsDirectory)

    if TestsDirectory != '' and FileSystem.is_dir(RelTestsPath)
        FindTestResult = run_command(
            'find', RelTestsPath, '-type', 'f', '(', '-name', '*.cpp', '-o', '-name', '*.c', ')',
            check : false
        )
        RawTestPaths = FindTestResult.stdout().strip()
        if RawTestPaths != ''
            foreach FilePath : RawTestPaths.split('\n')
                if FilePath != ''
                    TestSources += files(FilePath)
                endif
            endforeach
        endif
    endif

    if get_option('enable_testing') and TestSources.length() > 0
        HasCustomTestMain = FileSystem.is_file(RelTestsPath / 'Main.cpp') or FileSystem.is_file(RelTestsPath / 'TestMain.cpp')
        if not HasCustomTestMain
            TestSources += files(meson.project_source_root() / 'Meson' / 'TestRunner.cpp')
        endif

        TestIncludeDirectories = IncludeDirectories
        TestIncludeDirectories += include_directories(RelTestsPath)
        if FileSystem.is_dir(RelPrivatePath)
            TestIncludeDirectories += include_directories(RelPrivatePath)
        endif

        TestDependencies = [ModuleDependency]
        if ModuleName != 'Core' and GlobalModules.has_key('Core') and not ModuleDependencies.contains('Core')
            TestDependencies += GlobalModules.get('Core')
        endif

        TestTarget = executable(
            '@0@Tests'.format(ModuleName),
            TestSources,
            include_directories : TestIncludeDirectories,
            dependencies : TestDependencies,
            cpp_args : CompileArguments,
            c_args : CompileArguments,
            build_by_default : false
        )

        test(
            '@0@Tests'.format(ModuleName),
            TestTarget,
            workdir : meson.project_source_root(),
            suite : ModuleName
        )
    endif
endforeach