/**
 * @file RHIResourceRequirements.hpp
 * @brief Defines resource memory requirements.
 */
#pragma once
#include <Memory/RHIAllocationDesc.hpp>
#include <RHIExport.hpp>
#include <Templates/Optional.hpp>
namespace Lumen

{
/**
 * @struct FRHIResourceRequirements
 * @brief Size/alignment and acceptable memory classes.
 */
struct RHI_EXPORT FRHIResourceRequirements
{
    UInt64 Size      = 0ULL;
    UInt64 Alignment = 1ULL;
    TOptional<ERHIMemoryClass> PreferredMemoryClass;
    /** @return An allocation descriptor, or an error for invalid requirements. */
    [[nodiscard]] TExpected<FRHIAllocationDesc, FRHIError> ToAllocationDesc () const noexcept;
};
} // namespace Lumen
