LumenFindPackage(Vulkan)
if(NOT Vulkan_FOUND)
    message(STATUS "[Lumen] VulkanRHI: skipped (Vulkan not found)")
    return()
endif()

LumenModule(
    NAME            VulkanRHI
    TYPE            shared_library
    SOURCES         "Private/**/*.cpp"
    PUBLIC_INCLUDES "Public"
    DEFINES         LUMEN_ENGINE
    DEPS            Core MathCore
    LINUX_LIBS      Vulkan
)
