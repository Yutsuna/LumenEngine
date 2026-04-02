LumenFindPackage(Vulkan)
LumenModule(
    NAME            Renderer
    TYPE            shared_library
    SOURCES         "Private/**/*.cpp"
    PUBLIC_INCLUDES "Public"
    DEFINES         LUMEN_ENGINE
    DEPS            Core ApplicationCore MathCore VulkanRHI Parallel
)
