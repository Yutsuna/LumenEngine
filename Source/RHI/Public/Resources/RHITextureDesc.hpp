/**
 * @file RHITextureDesc.hpp
 * @brief Defines texture creation parameters.
 */

#pragma once

#include <RHIExport.hpp>

#include <Memory/RHIMemoryClass.hpp>
#include <Resources/RHIResourceUsage.hpp>

#include <Types/RHIClearValue.hpp>
#include <Types/RHIError.hpp>
#include <Types/RHIExtent.hpp>
#include <Types/RHIFormat.hpp>
#include <Types/RHIResourceState.hpp>
#include <Types/RHISampleCount.hpp>

namespace Lumen
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
    [[nodiscard]] TExpected<void, FRHIError> Validate () const noexcept;
};

} // namespace Lumen
