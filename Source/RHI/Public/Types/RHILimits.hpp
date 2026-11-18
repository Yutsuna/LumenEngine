/**
 * @file RHILimits.hpp
 * @brief Defines quantitative device limits.
 */
#pragma once

#include <Core/CoreTypes.hpp>
#include <RHIExport.hpp>
#include <Types/RHIError.hpp>

namespace Lumen
{

/**
 * @struct FRHILimits
 * @brief Quantitative limits guaranteed by an adapter or device.
 */
struct RHI_EXPORT FRHILimits
{
    UInt64 MaxBufferSize           = 0ULL;
    UInt32 MaxTextureDimension1D   = 0U;
    UInt32 MaxTextureDimension2D   = 0U;
    UInt32 MaxTextureDimension3D   = 0U;
    UInt32 MaxTextureArrayLayers   = 0U;
    UInt32 MaxColorAttachments     = 0U;
    UInt32 MaxBindingSets          = 0U;
    UInt32 MaxDescriptorsPerSet    = 0U;
    UInt32 MaxInlineConstantBytes  = 0U;
    UInt32 MaxVertexAttributes     = 0U;
    UInt32 MaxVertexBuffers        = 0U;
    UInt32 MaxComputeWorkgroupSize = 0U;
    UInt64 MinBufferAlignment      = 1ULL;
    UInt64 MinTextureRowAlignment  = 1ULL;
    UInt64 TimestampPeriod         = 1ULL;

    /** @return Success when all limits are internally coherent. */
    [[nodiscard]] TExpected<void, FRHIError> Validate () const noexcept;
};

} // namespace Lumen
