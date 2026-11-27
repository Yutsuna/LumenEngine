#{
# Source/RHI/meson.build
# Source/RHI meson entry-point
#}

RHIConformanceSources = files(
    'Tests/Device/RHIConformanceDeviceFactory.cpp',
    'Tests/Module/RHIModuleConformanceTests.cpp',
    'Tests/Device/RHIDeviceConformanceTests.cpp',
    'Tests/Memory/RHIMemoryConformanceTests.cpp',
    'Tests/Resources/RHIResourcesConformanceTests.cpp',
    'Tests/Bindings/RHIBindingsConformanceTests.cpp',
    'Tests/Shaders/RHIShadersConformanceTests.cpp',
    'Tests/Pipelines/RHIPipelinesConformanceTests.cpp',
    'Tests/Commands/RHICommandsConformanceTests.cpp',
    'Tests/Synchronization/RHISynchronizationConformanceTests.cpp',
    'Tests/Queue/RHIQueueConformanceTests.cpp',
    'Tests/Presentation/RHIPresentationConformanceTests.cpp',
    'Tests/Queries/RHIQueriesConformanceTests.cpp',
    'Tests/Caches/RHICachesConformanceTests.cpp',
    'Tests/Validation/RHIValidationConformanceTests.cpp',
)
RHIConformanceIncludeDirectories = include_directories('Tests')

ModuleDefinition = {
    'Name' : 'RHI',
    'Type' : 'SharedLibrary',
    'SourceDir' : meson.current_source_dir(),
    'Private' : 'Private',
    'Public' : 'Public',
    'Tests' : 'Tests',
    'Depends' : ['Core','Maths']
}
