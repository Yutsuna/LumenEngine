#{
# Source/RHIOpenGL/meson.build
# Source/RHIOpenGL meson entry-point
#}

# GL and EGL entry points are resolved at runtime (dlopen/eglGetProcAddress),
# so pkg-config dependencies are opportunistic: they supply include/link flags
# when available without making configuration fail on a stale pkg-config env.
OpenGLDependencies = []
foreach DependencyName : [ 'gl', 'egl' ]
    FoundDependency = dependency( DependencyName, required : false )
    if FoundDependency.found()
        OpenGLDependencies += FoundDependency
    endif
endforeach

ModuleDefinition = {
    'Name' : 'RHIOpenGL',
    'Type' : 'SharedLibrary',
    'SourceDir' : meson.current_source_dir(),
    'Private' : 'Private',
    'Public' : 'Public',
    'Tests' : 'Tests',
    'Depends' : ['Core', 'Maths', 'RHI'],
    'ExternalDeps' : OpenGLDependencies,

    # M3 conformance gate: all portable suites, including Synchronization, Queue,
    # Presentation and Queries on top of the M1/M2 coverage.
    'ExtraTestSources' : [
        RHIConformanceSources[0],
        RHIConformanceSources[1],
        RHIConformanceSources[2],
        RHIConformanceSources[3],
        RHIConformanceSources[4],
        RHIConformanceSources[5],
        RHIConformanceSources[6],
        RHIConformanceSources[7],
        RHIConformanceSources[8],
        RHIConformanceSources[9],
        RHIConformanceSources[10],
        RHIConformanceSources[11],
        RHIConformanceSources[12],
        RHIConformanceSources[13],
        RHIConformanceSources[14],
    ],

    'ExtraTestIncludeDirectories' : [ RHIConformanceIncludeDirectories ],
    'TestEnvironment' : [
        'LSAN_OPTIONS=suppressions=' + meson.current_source_dir() / 'lsan.supp',
    ],
}
