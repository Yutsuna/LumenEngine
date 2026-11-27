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

RHINullTestSupportSources = files(
    'Tests/Bindings/NullBindings.cpp',
    'Tests/Caches/NullCaches.cpp',
    'Tests/Commands/NullCommands.cpp',
    'Tests/Debug/RecordingDebugSink.cpp',
    'Tests/Device/NullConformanceDeviceFactory.cpp',
    'Tests/Device/NullDevice.cpp',
    'Tests/Device/NullFaultPlan.cpp',
    'Tests/Device/RHIConformanceDeviceFactory.cpp',
    'Tests/Memory/NullMemory.cpp',
    'Tests/Module/NullModule.cpp',
    'Tests/Pipelines/NullPipelines.cpp',
    'Tests/Presentation/NullPresentation.cpp',
    'Tests/Presentation/NullSurfaceBridge.cpp',
    'Tests/Queries/NullQueries.cpp',
    'Tests/Queue/NullQueue.cpp',
    'Tests/Resources/NullResources.cpp',
    'Tests/Shaders/NullShaders.cpp',
    'Tests/Synchronization/NullSynchronization.cpp',
)

ModuleDefinition = {
    'Name' : 'RHI',
    'Type' : 'SharedLibrary',
    'SourceDir' : meson.current_source_dir(),
    'Private' : 'Private',
    'Public' : 'Public',
    'Tests' : 'Tests',
    'Depends' : ['Core','Maths']
}
