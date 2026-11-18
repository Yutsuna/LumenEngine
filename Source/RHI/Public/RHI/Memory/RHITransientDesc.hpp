/**
 * @file RHITransientDesc.hpp
 * @brief Defines one transient memory/resource request.
 */
#pragma once

#include <RHI/Memory/RHIResourceLifetime.hpp>
#include <RHI/Memory/RHIResourceRequirements.hpp>

namespace Lumen
{

namespace RHI
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
} // namespace RHI

} // namespace Lumen
