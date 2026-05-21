LumenModule(
    NAME            Renderer
    TYPE            shared_library
    SOURCES         "Private/**/*.cpp"
    PUBLIC_INCLUDES "Public"
    DEFINES         LUMEN_ENGINE
    DEPS            Core ApplicationCore MathCore RHI Parallel Compiler
)

########################################################

target_compile_definitions(
    VulkanRHI
    PRIVATE
        LUMEN_GPU_CULL_SHADER_PATH="${CMAKE_SOURCE_DIR}/LumenEngine/Shaders/GpuCulling.comp"
)

########################################################