#{
# Source/Meson/Benchmarks/meson.build
# Discovers and registers each module's benchmark suite.
# Reads  : ModuleDefinitions, GlobalModules, PublicIncludes, ALL_WARNINGS
# Writes : (nothing; only registers `benchmark()` targets)
#}

if not get_option('enable_benchmarking')
    subdir_done()
endif

foreach ModuleDefinition : ModuleDefinitions
    ModuleName = ModuleDefinition.get('Name')
    ModuleSourceDirRel = ModuleDefinition.get('SourceDirRel')
    ModuleDependencies = ModuleDefinition.get('Depends', [])

    ModulePath = SourceRoot / ModuleSourceDirRel
    PrivatePath = ModulePath / ModuleDefinition.get('Private', 'Private')
    BenchmarksPath = ModulePath / ModuleDefinition.get('Benchmarks', 'Benchmarks')

    BenchmarkSources = []

    if FileSystem.is_dir(BenchmarksPath)
        FindBenchmarkResult = run_command(
            'find', BenchmarksPath,
            '-type', 'f', '(', '-name', '*.cpp', '-o', '-name', '*.c', ')',
            check : false,
        )
        foreach FilePath : FindBenchmarkResult.stdout().strip().split('\n')
            if FilePath != ''
                BenchmarkSources += files(FilePath)
            endif
        endforeach
    endif

    if BenchmarkSources.length() > 0
        HasCustomBenchmarkMain = FileSystem.is_file(BenchmarksPath / 'Main.cpp')\
or FileSystem.is_file(BenchmarksPath / 'BenchmarkMain.cpp')

        if not HasCustomBenchmarkMain
            BenchmarkSources += files(meson.project_source_root() / 'Meson' / 'BenchRunner.cpp')
        endif

        PrivateDirectory = ModuleDefinition.get('Private', 'Private')
        BenchmarksDirectory = ModuleDefinition.get('Benchmarks', 'Benchmarks')

        RelPrivatePath = '../../@0@/@1@'.format(ModuleSourceDirRel, PrivateDirectory)
        RelBenchmarksPath = '../../@0@/@1@'.format(ModuleSourceDirRel, BenchmarksDirectory)

        BenchmarkIncludeDirectories = PublicIncludes.get(ModuleName, [])
        BenchmarkIncludeDirectories += include_directories(RelBenchmarksPath)

        if FileSystem.is_dir(RelPrivatePath)
            BenchmarkIncludeDirectories += include_directories(RelPrivatePath)
        endif

        BenchmarkDependencies = [GlobalModules.get(ModuleName)]

        if ModuleName != 'Core' and GlobalModules.has_key('Core')\
and not ModuleDependencies.contains('Core')
            BenchmarkDependencies += GlobalModules.get('Core')
        endif

        CompileArguments = ALL_WARNINGS

        BenchmarkTarget = executable(
            '@0@Benchmarks'.format(ModuleName),
            BenchmarkSources,
            include_directories : BenchmarkIncludeDirectories,
            dependencies : BenchmarkDependencies,
            cpp_args : CompileArguments,
            c_args : CompileArguments,
            build_by_default : false,
        )

        benchmark(
            '@0@Benchmarks'.format(ModuleName),
            BenchmarkTarget,
            workdir : meson.project_source_root(),
            suite : ModuleName,
            timeout : 0, #<< Benchmarks are long by nature; the 30s default kills whole suites.
        )
    endif
endforeach
