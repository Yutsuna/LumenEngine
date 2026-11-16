/**
 * @file RHITransientDesc.hpp
 * @brief Defines one transient memory/resource request.
 */
#pragma once
#include <Memory/RHIResourceLifetime.hpp>
#include <Memory/RHIResourceRequirements.hpp>
namespace Lumen

{
/**
 * @struct FRHITransientDesc
 * @brief Requirements and live interval of a transient object.
 */
struct FRHITransientDesc
{
    FRHIResourceRequirements Requirements;
    FRHIResourceLifetime Lifetime;
    /** @return True when requirements and lifetime are valid. */
    [[nodiscard]] Bool Validate () const noexcept;
};
} // namespace Lumen
