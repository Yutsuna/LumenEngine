/**
 * @file VulkanRHIDynamic.hpp
 * @brief Vulkan RHI dynamic resources
 */

#pragma once

#include <vulkan/vulkan.h>

#include "Generic/RHIDynamic.hpp"
#include "Generic/RHIViewport.hpp"

#include "Maths/Vector.hpp"

namespace LumenEngine
{

namespace RHI
{

    class LUMEN_ENGINE_API FVulkanRHIDynamic final : public IFRHIDynamic
    {
    public:

        FVulkanRHIDynamic ();
        ~FVulkanRHIDynamic () override;

    public:

        void Init () override;
        void Shutdown () override;

        TSharedRef<IFRHIViewport> RHICreateViewport ( void *InWindowHandle, const Maths::FVec2u &InSize ) override;
        VkInstance GetVulkanInstance () const;

    private:

        void CreateVulkanInstance ();

    private:

        VkInstance VulkanInstance;
    };

} // namespace RHI

} // namespace LumenEngine
