/**
 * @file VulkanResource.hpp
 * @brief Base classes for RHI resources
 */

#pragma once

#include "CoreTypes.hpp"

namespace LumenEngine
{

namespace RHI
{

    /**
     * @class IRHIResource
     * @brief Interface for all RHI resources
     */
    class IRHIResource
    {
    public:

        IRHIResource ()          = default;
        virtual ~IRHIResource () = default;

    public:

        /**
         * @brief Releases the resource immediately
         */
        virtual void Release () = 0;

        /**
         * @brief Defers the release of the resource until it's safe to destroy (GPU finished processing)
         */
        virtual void DeferredRelease () = 0;
    };

    /**
     * @class FVulkanResource
     * @brief Base class for Vulkan RHI resources with deferred deletion support
     */
    class FVulkanResource : public IRHIResource
    {
    public:

        FVulkanResource () noexcept;
        ~FVulkanResource () noexcept override;

    public:

        /** @brief Immediate release of the resource */
        void Release () override = 0;

        /** @brief Default implementation of deferred release via FVulkanRHI */
        void DeferredRelease () override;

    protected:

        /** Frame index when the resource was requested for deletion */
        UInt64 DeletionFrame{ 0 };
    };

} // namespace RHI

} // namespace LumenEngine
