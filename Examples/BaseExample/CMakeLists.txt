LumenModule(
    NAME            BaseExample
    TYPE            executable
    SOURCES         "Private/*.cpp"
    PUBLIC_INCLUDES "Public"
    DEFINES         LUMEN_ENGINE
    DEPS            Launch ApplicationCore VulkanRHI
)
