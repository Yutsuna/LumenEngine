/**
 * @file VulkanPipelineCache.hpp
 * @brief Vulkan Pipeline Cache RHI wrapper
 */

#pragma once

#include "Container/Map.hpp"
#include "CoreTypes.hpp"
#include <vulkan/vulkan_core.h>

namespace LumenEngine
{

namespace RHI
{

    class FVulkanDevice;

    /**
     * @class FVulkanPipelineCache
     * @brief Manages the creation and reuse of VkPipeline objects
     */
    class FVulkanPipelineCache final
    {
    public:

        FVulkanPipelineCache ( FVulkanDevice &InDevice ) noexcept;
        ~FVulkanPipelineCache () noexcept;

    public:

        /**
         * @brief Retrieves a graphics pipeline from the cache or creates a new one
         * @param InCreateInfo Information required to create the pipeline
         * @return The created or cached VkPipeline handle
         */
        [[nodiscard]] VkPipeline GetOrCreateGraphicsPipeline ( const VkGraphicsPipelineCreateInfo &InCreateInfo );

        /**
         * @brief Retrieves a compute pipeline from the cache or creates a new one
         * @param InCreateInfo Information required to create the pipeline
         * @return The created or cached VkPipeline handle
         */
        [[nodiscard]] VkPipeline GetOrCreateComputePipeline ( const VkComputePipelineCreateInfo &InCreateInfo );

        /** @brief Clears the cache and destroys all pipelines */
        void Clear ();

    private:

        /** Computes a hash for a graphics pipeline create info to use as a cache key */
        [[nodiscard]] UInt64 HashGraphicsPipeline ( const VkGraphicsPipelineCreateInfo &InCreateInfo ) const noexcept;

        /** Computes a hash for a compute pipeline create info to use as a cache key */
        [[nodiscard]] UInt64 HashComputePipeline ( const VkComputePipelineCreateInfo &InCreateInfo ) const noexcept;

    private:

        /** Device reference used to create and destroy pipelines */
        FVulkanDevice &Device;

        /** Cache of graphics pipelines, keyed by hash */
        TMap<UInt64, VkPipeline> GraphicsPipelines;

        /** Cache of compute pipelines, keyed by hash */
        TMap<UInt64, VkPipeline> ComputePipelines;

        /** Vulkan pipeline cache object for internal driver optimization */
        VkPipelineCache PipelineCache{ VK_NULL_HANDLE };
    };

} // namespace RHI

} // namespace LumenEngine
