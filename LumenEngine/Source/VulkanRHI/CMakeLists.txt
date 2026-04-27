LumenFindPackage(Vulkan)
LumenFindPackage(VulkanMemoryAllocator)

###########################################################

if(NOT Vulkan_FOUND)
    message(STATUS "[Lumen] VulkanRHI: skipped (Missing Vulkan)")
    return()
endif()

if(NOT VulkanMemoryAllocator_FOUND)
    message(STATUS "[Lumen] VulkanRHI: skipped (Missing VulkanMemoryAllocator)")
    return()
endif()

###########################################################

LumenModule(
    NAME            VulkanRHI
    TYPE            shared_library
    SOURCES         "Private/**/*.cpp"
    PUBLIC_INCLUDES "Public"
    DEFINES         LUMEN_ENGINE
    DEPS            Core ApplicationCore MathCore RHI
    GLSLang         SPIRV-Tools SPIRV-Cross-Core SPIRV-Cross-GLSL
)

###########################################################

LumenCompileShaders(
    VulkanRHI
    Shaders
    SOURCES
        "${CMAKE_SOURCE_DIR}/LumenEngine/Shaders/GpuCulling.comp"
)

target_compile_definitions(
    VulkanRHI
    PRIVATE
        LUMEN_GPU_CULL_SHADER_PATH="${CMAKE_CURRENT_BINARY_DIR}/Shaders/GpuCulling.comp.spv"
)
