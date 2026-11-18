/**
 * @file GenericTime.hpp
 * @brief Platform-independent time constants shared by every FPlatformTime implementation.
 */

#pragma once

#include <Core/CoreTypes.hpp>
#include <CoreExport.hpp>

namespace Lumen
{

namespace HAL
{

    /**
     * @struct FGenericPlatformTime
     * @brief Base of every platform time implementation. Holds constants only.
     */
    struct CORE_EXPORT FGenericPlatformTime
    {
        static constexpr Float64 DefaultTickRate = 1.0 / 60.0;
        static constexpr Float64 MaxTickRate     = 1.0 / 15.0;

        /** Upper bound (in seconds) accepted by Sleep(). */
        static constexpr Float64 MaxSleepSeconds = 4.0e6;

        template <Float64 Factor> [[nodiscard]] static constexpr Float64 ToSeconds ( UInt64 Value ) noexcept
        {
            return static_cast<Float64>( Value ) * Factor;
        }
    };

} // namespace HAL

} // namespace Lumen
