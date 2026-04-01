LumenFindPackage(Vulkan)
LumenFindPackage(VulkanMemoryAllocator)
LumenFindPackage(vk-bootstrap)

###########################################################

if(NOT Vulkan_FOUND)
    message(STATUS "[Lumen] VulkanRHI: skipped (Missing Vulkan)")
    return()
endif()

if(NOT VulkanMemoryAllocator_FOUND)
    message(STATUS "[Lumen] VulkanRHI: skipped (Missing VulkanMemoryAllocator)")
    return()
endif()

if(NOT vk-bootstrap_FOUND)
    message(STATUS "[Lumen] VulkanRHI: skipped (Missing vk-bootstrap)")
    return()
endif()

###########################################################

LumenModule(
    NAME            VulkanRHI
    TYPE            shared_library
    SOURCES         "Private/**/*.cpp"
    PUBLIC_INCLUDES "Public"
    DEFINES         LUMEN_ENGINE
    DEPS            Core ApplicationCore MathCore
    LINUX_LIBS      Vulkan::Vulkan GPUOpen::VulkanMemoryAllocator vk-bootstrap::vk-bootstrap
)
