/**
 * @file RHITextureDesc.hpp
 * @brief Defines texture creation parameters.
 */

#pragma once

#include <RHIExport.hpp>

#include <RHI/Memory/RHIMemoryClass.hpp>
#include <RHI/Resources/RHIResourceUsage.hpp>

#include <RHI/Types/RHIClearValue.hpp>
#include <RHI/Types/RHIError.hpp>
#include <RHI/Types/RHIExtent.hpp>
#include <RHI/Types/RHIFormat.hpp>
#include <RHI/Types/RHIResourceState.hpp>
#include <RHI/Types/RHISampleCount.hpp>

namespace Lumen
{

namespace RHI
{

    /**
     * @struct FRHITextureDesc
     * @brief Immutable texture dimensions and usage.
     */
    struct RHI_EXPORT FRHITextureDesc
    {
        FRHIExtent3D Extent;
        ERHIFormat Format              = ERHIFormat::Unknown;
        UInt32 MipCount                = 1U;
        UInt32 LayerCount              = 1U;
        ERHISampleCount SampleCount    = ERHISampleCount::Count1;
        ERHIResourceUsage Usage        = ERHIResourceUsage::None;
        ERHIMemoryClass MemoryClass    = ERHIMemoryClass::DeviceLocal;
        ERHIResourceState InitialState = ERHIResourceState::Common;

        FRHIClearValue OptimizedClear;

        /** @return Success when dimensions, format, counts, usage, and state are valid. */
        [[nodiscard]] FRHIExpected Validate () const noexcept;
    };

} // namespace RHI

} // namespace Lumen
