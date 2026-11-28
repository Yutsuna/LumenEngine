#{
# Source/Renderer/meson.build
# Source/Renderer meson entry-point
#}

ModuleDefinition = {
    'Name' : 'Renderer',
    'Type' : 'SharedLibrary',
    'SourceDir' : meson.current_source_dir(),
    'Private' : 'Private',
    'Public' : 'Public',
    'Tests' : 'Tests',
    'Benchmarks' : 'Benchmarks',
    'Depends' : ['Core', 'Maths', 'Scene'],
    'PrivateDepends' : ['RHI', 'RHIOpenGL'],
    'ExtraTestSources' : RHINullTestSupportSources,
    'ExtraTestIncludeDirectories' : [ RHIConformanceIncludeDirectories, RHIPrivateIncludeDirectories ],

    'ExtraBenchmarkSources' : RHINullTestSupportSources,
    'ExtraBenchmarkIncludeDirectories' : [ RHIConformanceIncludeDirectories, RHIPrivateIncludeDirectories ],
    'BenchmarkTestDependency' : true,
}
