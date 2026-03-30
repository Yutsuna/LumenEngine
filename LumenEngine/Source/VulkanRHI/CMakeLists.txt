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
    SOURCES         "Private/*.cpp"
    PUBLIC_INCLUDES "Public"
    DEFINES         LUMEN_ENGINE
    DEPS            Core MathCore
    LINUX_LIBS      Vulkan::Vulkan GPUOpen::VulkanMemoryAllocator
)
