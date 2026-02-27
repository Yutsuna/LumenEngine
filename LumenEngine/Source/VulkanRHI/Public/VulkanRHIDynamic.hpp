/**
 * @file VulkanRHIDynamic.hpp
 * @brief Vulkan RHI dynamic resources
 */

#pragma once

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

        TSharedPtr<IFRHIViewport> CreateViewport ( void *InWindowHandle, const LumenEngine::Math::FVec2u &InSize ) override;
        VKInstance GetVulkanInstance () const;

    private:

        void CreateVulkanInstance ();

    private:

        VKInstance VulkanInstance;
    };

} // namespace RHI

} // namespace LumenEngine
