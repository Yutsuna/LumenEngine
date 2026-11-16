/**
 * @file RHITransientBatch.hpp
 * @brief Defines a group of transient requests.
 */
#pragma once
#include <Memory/RHITransientDesc.hpp>
#include <RHIExport.hpp>
#include <Templates/Vector.hpp>
namespace Lumen

{
/**
 * @struct FRHITransientBatch
 * @brief Requests placed together with aliasing analysis.
 */
struct RHI_EXPORT FRHITransientBatch
{
    TVector<FRHITransientDesc> Requests;
    /** @return Success when every request is valid. */
    [[nodiscard]] TExpected<void, FRHIError> Validate () const noexcept;
};
} // namespace Lumen
