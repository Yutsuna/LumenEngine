/**
 * @file RHIShaderReflection.hpp
 * @brief Defines backend-independent shader interface metadata.
 */
#pragma once
#include <RHIExport.hpp>
#include <Templates/Array.hpp>
#include <Templates/Vector.hpp>
#include <Types/RHIError.hpp>
#include <Types/RHIShaderStage.hpp>
namespace Lumen
{
/** @enum ERHIShaderBindingKind @brief Resource classes found by reflection. */
enum class ERHIShaderBindingKind : UInt8
{
    ConstantBuffer,
    ReadOnlyBuffer,
    ReadWriteBuffer,
    Texture,
    StorageTexture,
    Sampler

};
/**
 * @struct FRHIShaderBindingReflection
 * @brief Binding slot, resource kind, count, and stages.
 */
struct FRHIShaderBindingReflection
{
    UInt32 Slot                = 0U;
    ERHIShaderBindingKind Kind = ERHIShaderBindingKind::ReadOnlyBuffer;
    UInt32 Count               = 1U;
    ERHIShaderStage Stages     = ERHIShaderStage::None;
};
/**
 * @struct FRHIShaderReflection
 * @brief Bindings and compute workgroup metadata.
 */
struct RHI_EXPORT FRHIShaderReflection
{
    TVector<FRHIShaderBindingReflection> Bindings;
    TArray<UInt32, 3U> WorkgroupSize = { 1U, 1U, 1U };
    UInt32 InlineConstantBytes       = 0U;
    /** @return Success when slots are unique and counts nonzero. */
    [[nodiscard]] TExpected<void, FRHIError> Validate () const noexcept;
};
} // namespace Lumen
