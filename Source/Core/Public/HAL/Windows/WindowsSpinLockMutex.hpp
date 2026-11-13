/**
 * @file WindowsSpinLockMutex.hpp
 * @brief Busy-wait spin lock for Windows.
 */

#pragma once

#include <Core/CoreDefinitions.hpp>

#if not defined( LUMEN_PLATFROM_WINDOWS )
    #error "WindowsMutex.hpp is only available on Windows platforms."
#else

    #include <Core/CoreTypes.hpp>
    #include <Core/NonCopyable.hpp>
    #include <Core/NonMovable.hpp>

    #include <HAL/PlatformAtomic.hpp>

    #include <atomic>

extern "C"
{
    #ifndef WIN32_LEAN_AND_MEAN
        #define WIN32_LEAN_AND_MEAN
    #endif
    #ifndef NOMINMAX
        #define NOMINMAX
    #endif
    #include <windows.h>
}

namespace Lumen
{

namespace HAL
{

    /**
     * @class FWindowsSpinLockMutex
     * @brief TTAS spin lock. Relaxes the CPU first, then yields the time slice.
     * @details Never parks the thread. Only for very short critical sections.
     */
    class CORE_EXPORT FWindowsSpinLockMutex final : public FNonCopyable, public FNonMovable
    {
    public:

        FWindowsSpinLockMutex () noexcept  = default;
        ~FWindowsSpinLockMutex () noexcept = default;

    public:

        void Lock () noexcept;
        void Unlock () noexcept;

        [[nodiscard]] Bool IsLocked () const noexcept;
        [[nodiscard]] Bool TryLock () noexcept;

    private:

        /** @brief Number of CpuRelax() iterations before the waiter starts yielding its time slice. */
        static constexpr UInt32 GMaxRelaxSpins = 64U;

        std::atomic_flag Flag{};
    };

} // namespace HAL

} // namespace Lumen

    #include "Inline/WindowsSpinLockMutex.inl"

#endif
