#{
# Source/Meson/Tests/meson.build
# Discovers and registers each module's test suite.
#
# Reads  : ModuleDefinitions, GlobalModules, PublicIncludes, ALL_WARNINGS
# Writes : (nothing; only registers `test()` targets)
#}

if not get_option('enable_testing')
    subdir_done()
endif

foreach ModuleDefinition : ModuleDefinitions
    ModuleName = ModuleDefinition.get('Name')
    ModuleSourceDirRel = ModuleDefinition.get('SourceDirRel')
    ModuleDependencies = ModuleDefinition.get('Depends', [])

    ModulePath = SourceRoot / ModuleSourceDirRel
    PrivatePath = ModulePath / ModuleDefinition.get('Private', 'Private')
    TestsPath = ModulePath / ModuleDefinition.get('Tests', 'Tests')

    TestSources = []

    if FileSystem.is_dir(TestsPath)
        FindTestResult = run_command(
            'find', TestsPath,
            '-type', 'f', '(', '-name', '*.cpp', '-o', '-name', '*.c', ')',
            check : false,
        )
        foreach FilePath : FindTestResult.stdout().strip().split('\n')
            if FilePath != ''
                TestSources += files(FilePath)
            endif
        endforeach
    endif

    if TestSources.length() > 0
        HasCustomTestMain = FileSystem.is_file(TestsPath / 'Main.cpp')\
or FileSystem.is_file(TestsPath / 'TestMain.cpp')

        if not HasCustomTestMain
            TestSources += files(meson.project_source_root() / 'Meson' / 'TestRunner.cpp')
        endif

        PrivateDirectory = ModuleDefinition.get('Private', 'Private')
        TestsDirectory = ModuleDefinition.get('Tests', 'Tests')

        RelPrivatePath = '../../@0@/@1@'.format(ModuleSourceDirRel, PrivateDirectory)
        RelTestsPath = '../../@0@/@1@'.format(ModuleSourceDirRel, TestsDirectory)

        TestIncludeDirectories = PublicIncludes.get(ModuleName, [])
        TestIncludeDirectories += include_directories(RelTestsPath)

        if FileSystem.is_dir(RelPrivatePath)
            TestIncludeDirectories += include_directories(RelPrivatePath)
        endif

        TestDependencies = [GlobalModules.get(ModuleName)]

        if ModuleName != 'Core' and GlobalModules.has_key('Core')\
and not ModuleDependencies.contains('Core')
            TestDependencies += GlobalModules.get('Core')
        endif

        CompileArguments = ALL_WARNINGS

        TestTarget = executable(
            '@0@Tests'.format(ModuleName),
            TestSources,
            include_directories : TestIncludeDirectories,
            dependencies : TestDependencies,
            cpp_args : CompileArguments,
            c_args : CompileArguments,
            build_by_default : false,
        )

        test(
            '@0@Tests'.format(ModuleName),
            TestTarget,
            workdir : meson.project_source_root(),
            suite : ModuleName,
        )
    endif
endforeach