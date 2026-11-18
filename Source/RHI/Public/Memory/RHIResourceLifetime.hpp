/**
 * @file RHIResourceLifetime.hpp
 * @brief Defines GPU resource lifetime intervals.
 */
#pragma once

#include <Core/CoreTypes.hpp>
#include <RHIExport.hpp>

namespace Lumen

{
/**
 * @struct FRHIResourceLifetime
 * @brief Half-open frame interval used by transient aliasing.
 */
struct RHI_EXPORT FRHIResourceLifetime
{
    UInt64 Begin = 0ULL;
    UInt64 End   = 0ULL;
    /** @return True when End is not before Begin. */
    [[nodiscard]] constexpr Bool IsValid () const noexcept
    {
        return End >= Begin;
    }
    /** @return True when two nondegenerate live intervals overlap. */
    [[nodiscard]] Bool Overlaps ( const FRHIResourceLifetime &InOther ) const noexcept;
};
} // namespace Lumen
