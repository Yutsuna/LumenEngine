/**
 * @file VulkanShader.hpp
 * @brief Vulkan shader resource RHI
 */

#pragma once

#include "Container/Vector.hpp"
#include "CoreTypes.hpp"

namespace LumenEngine
{

namespace RHI
{

    struct FVulkanShader
    {

        UInt32 DescriptorID;
        TVector<UInt32> Bindings;

        enum class EAttachmentType : UInt8
        {
            None,
            UniformBuffer,
            StorageBuffer,
            SampledImage,
            StorageImage,
            Sampler
        };

        struct FInputAttachment
        {
            UInt8 Binding;
            EAttachmentType Type;
        };
    };

} // namespace RHI

} // namespace LumenEngine
