/**
 * @file RHIDrawArguments.hpp
 * @brief Defines direct, indirect, and dispatch arguments.
 */

#pragma once
#include <RHIExport.hpp>
#include <Types/RHIError.hpp>
namespace Lumen
{
enum class ERHIIndexFormat : UInt8
{
    Index16,
    Index32
};
struct RHI_EXPORT FRHIDrawArguments
{
    UInt32 VertexCount   = 0U;
    UInt32 InstanceCount = 1U;
    UInt32 FirstVertex   = 0U;
    UInt32 FirstInstance = 0U;
    [[nodiscard]] TExpected<void, FRHIError> Validate () const noexcept;
};
struct RHI_EXPORT FRHIDrawIndexedArguments
{
    UInt32 IndexCount    = 0U;
    UInt32 InstanceCount = 1U;
    UInt32 FirstIndex    = 0U;
    Int32 VertexOffset   = 0;
    UInt32 FirstInstance = 0U;
    [[nodiscard]] TExpected<void, FRHIError> Validate () const noexcept;
};
struct RHI_EXPORT FRHIDispatchArguments
{
    UInt32 GroupCountX = 1U;
    UInt32 GroupCountY = 1U;
    UInt32 GroupCountZ = 1U;
    [[nodiscard]] TExpected<void, FRHIError> Validate ( UInt32 InMaximumPerDimension = 65535U ) const noexcept;
};
struct RHI_EXPORT FRHIIndirectArguments
{
    UInt64 Offset = 0ULL;
    UInt32 Count  = 1U;
    UInt32 Stride = 0U;
    /** @brief Validates alignment, count, stride, and the complete byte range. */
    [[nodiscard]] TExpected<void, FRHIError> Validate ( UInt64 InBufferSize, UInt32 InMinimumStride ) const noexcept;
};
} // namespace Lumen
