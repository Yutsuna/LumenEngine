/**
 * @file VulkanPipelineCache.hpp
 * @brief Declaration of the FVulkanPipelineCache class for managing Vulkan Pipeline Caches.
 */

#include "Definitions.hpp"

#include "NonCopyable.hpp"

#include "Filesystem/Path.hpp"

#include <vulkan/vulkan_core.h>

namespace LumenEngine
{

namespace VulkanRHI
{

    /**
     * @class FVulkanPipelineCache
     * @brief Manages a Vulkan VkPipelineCache object with disk serialization
     */
    class LUMEN_ENGINE_API FVulkanPipelineCache final : public FNonCopyable
    {
    public:

        FVulkanPipelineCache () noexcept  = default;
        ~FVulkanPipelineCache () noexcept = default;

        FVulkanPipelineCache ( FVulkanPipelineCache &&InOther ) noexcept;
        FVulkanPipelineCache &operator=( FVulkanPipelineCache &&InOther ) noexcept;

    public:

        /**
         * @brief Initializes the Vulkan pipeline cache by loading binary data from disk.
         * @param InDevice The Vulkan logical device.
         * @param InCachePath File path to store and retrieve cache data.
         */
        void Initialize ( VkDevice InDevice, const Filesystem::FPath &InCachePath ) noexcept;

        /**
         * @brief Queries Vulkan for the latest serialized cache data and writes it to disk.
         * @param InDevice The Vulkan logical device.
         */
        void SaveToDisk ( VkDevice InDevice ) noexcept;

        /**
         * @brief Destroys the Vulkan pipeline cache object.
         * @param InDevice The Vulkan logical device.
         */
        void Shutdown ( VkDevice InDevice ) noexcept;

        /** @return The underlying Vulkan pipeline cache handle */
        [[nodiscard]] VkPipelineCache GetHandle () const noexcept;

    private:

        VkPipelineCache CacheHandle = VK_NULL_HANDLE;
        Filesystem::FPath CacheFilePath;
    };

} // namespace VulkanRHI

} // namespace LumenEngine
