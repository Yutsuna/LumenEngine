/**
 * @file WindowsFutex.hpp
 * @brief Address-based wait / wake primitive.
 */

#pragma once

#include <Core/CoreDefinitions.hpp>

#if not defined( LUMEN_PLATFORM_WINDOWS )
    #error "WindowsFutex.hpp is only available on Windows."
#else

    #include <Core/CoreTypes.hpp>
    #include <CoreExport.hpp>

namespace Lumen
{

namespace HAL
{

    /**
     * @struct FWindowsFutex
     * @brief Parks the calling thread while a 32-bit atomic still holds an expected value.
     */
    struct CORE_EXPORT FWindowsFutex
    {
        /** @brief Sleeps while InAddress == InExpected. */
        static void Wait ( TAtomic<UInt32> &InAddress, UInt32 InExpected ) noexcept;

        /**
         * @brief Same as Wait() with a relative timeout.
         * @param InTimeoutSeconds Relative timeout in seconds (<= 0 means "do not block").
         * @return False if (and only if) the timeout elapsed, true otherwise (woken, value differed, or spurious).
         */
        [[nodiscard]] static Bool WaitFor ( TAtomic<UInt32> &InAddress, UInt32 InExpected, Float64 InTimeoutSeconds ) noexcept;

        /** @brief Wakes at most one thread parked on InAddress. */
        static void WakeOne ( TAtomic<UInt32> &InAddress ) noexcept;

        /** @brief Wakes every thread parked on InAddress. */
        static void WakeAll ( TAtomic<UInt32> &InAddress ) noexcept;

    private:

        /** @brief The raw 32-bit word behind the atomic (layout checked by the static_asserts above). */
        static UInt32 *Raw ( TAtomic<UInt32> &InAddress ) noexcept;

        /** @brief Clamps to [0, 1 year]. NaN becomes 0. */
        static Float64 ClampTimeout ( Float64 InSeconds ) noexcept;
    };

} // namespace HAL

} // namespace Lumen

#endif
