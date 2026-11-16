/**
 * @file RHICopyFootprint.hpp
 * @brief Defines linear layouts for buffer-texture copies.
 */
#pragma once

#include <Core/CoreTypes.hpp>
#include <RHIExport.hpp>
#include <Templates/Expected.hpp>
#include <Types/RHIError.hpp>
#include <Types/RHIExtent.hpp>
#include <Types/RHIFormat.hpp>

namespace Lumen
{

/**
 * @struct FRHICopyFootprint
 * @brief Byte layout of one linear texture copy.
 */
struct FRHICopyFootprint
{
    UInt64 Offset     = 0ULL;
    UInt64 RowPitch   = 0ULL;
    UInt64 SlicePitch = 0ULL;
    UInt64 TotalSize  = 0ULL;
    UInt32 RowCount   = 0U;
    UInt32 SliceCount = 0U;
};

/** @brief Computes a tightly packed or explicitly row-aligned footprint. */
[[nodiscard]] RHI_EXPORT TExpected<FRHICopyFootprint, FRHIError> ComputeRHICopyFootprint (
    ERHIFormat InFormat, FRHIExtent3D InExtent, UInt64 InOffset = 0ULL, UInt64 InRowAlignment = 1ULL ) noexcept;

} // namespace Lumen
