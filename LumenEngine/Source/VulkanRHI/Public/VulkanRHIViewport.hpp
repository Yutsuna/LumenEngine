/**
 * @file VulkanRHIViewport.hpp
 * @brief Vulkan RHI viewport
 */

#pragma once

#include "CoreTypes.hpp"
#include "Definitions.hpp"

#include "Generic/RHIViewport.hpp"

namespace LumenEngine
{

namespace RHI
{

    class LUMEN_ENGINE_API FVulkanRHIViewport final : public IFRHIViewport
    {
    public:

        FVulkanRHIViewport ();
        ~FVulkanRHIViewport () override;

    public:

        /** Retrieves the width of the viewport. */
        UInt32 GetWidth () const override;

        /** Retrieves the height of the viewport. */
        UInt32 GetHeight () const override;
    };

} // namespace RHI

} // namespace LumenEngine
