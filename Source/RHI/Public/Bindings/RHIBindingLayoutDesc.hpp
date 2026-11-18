/**
 * @file RHIBindingLayoutDesc.hpp
 * @brief Defines immutable binding layout entries.
 */
#pragma once
#include <Bindings/RHIBindingType.hpp>
#include <RHIExport.hpp>
#include <Templates/Vector.hpp>
#include <Types/RHIError.hpp>
#include <Types/RHIHash.hpp>
#include <Types/RHIShaderStage.hpp>
namespace Lumen

{
/**
 * @struct FRHIBindingLayoutEntry
 * @brief Slot type, descriptor count, and visible stages.
 */
struct FRHIBindingLayoutEntry
{
    UInt32 Slot            = 0U;
    ERHIBindingType Type   = ERHIBindingType::ReadOnlyBuffer;
    UInt32 Count           = 1U;
    ERHIShaderStage Stages = ERHIShaderStage::All;
    Bool bBindless         = false;
};
/**
 * @struct FRHIBindingLayoutDesc
 * @brief Canonical collection of binding entries.
 */
struct RHI_EXPORT FRHIBindingLayoutDesc
{
    TVector<FRHIBindingLayoutEntry> Entries;
    /** @return Success for unique slots and nonzero counts. */
    [[nodiscard]] TExpected<void, FRHIError> Validate () const noexcept;
    /** @return Stable content hash independent of entry ordering. */
    [[nodiscard]] UInt64 GetHash () const noexcept;
};
} // namespace Lumen
