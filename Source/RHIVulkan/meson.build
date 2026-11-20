#{
# Source/RHIVulkan/meson.build
# Source/RHIVulkan meson entry-point
#}

VulkanDependencies = []
VulkanDependency = dependency('vulkan', required : false)

if VulkanDependency.found()
    VulkanDependencies += VulkanDependency
endif

ModuleDefinition = {
    'Name' : 'RHIVulkan',
    'Type' : 'SharedLibrary',
    'SourceDir' : meson.current_source_dir(),
    'Private' : 'Private',
    'Public' : 'Public',
    'Tests' : 'Tests',
    'Depends' : [ 'Core', 'Maths', 'RHI' ],
    'ExternalDeps' : VulkanDependencies,
}
