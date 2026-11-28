/**
 * @file WindowsSpinLockMutex.hpp
 * @brief Busy-wait spin lock for Windows.
 */

#pragma once

#include <Core/CoreDefinitions.hpp>

#if not defined( LUMEN_PLATFORM_WINDOWS )
    #if not defined( LUMEN_CLANG_TIDY )
        #error "WindowsSpinLockMutex.hpp is only supported on Windows"
    #endif
#else

    #include <Core/CoreTypes.hpp>
    #include <Core/NonCopyable.hpp>
    #include <Core/NonMovable.hpp>

    #include <HAL/PlatformAtomic.hpp>

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

/* winbase.h defines CreateSemaphore as CreateSemaphoreA/W, which collides with
 * IRHIDevice::CreateSemaphore and its overrides. No Core HAL code calls the Win32
 * function, so drop the legacy macro before any engine declaration. */
    #undef CreateSemaphore

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

        LUMEN_FORCE_INLINE void Lock () noexcept;
        LUMEN_FORCE_INLINE void Unlock () noexcept;

        [[nodiscard]] LUMEN_FORCE_INLINE Bool IsLocked () const noexcept;
        [[nodiscard]] LUMEN_FORCE_INLINE Bool TryLock () noexcept;

    private:

        /** @brief Number of CpuRelax() iterations before the waiter starts yielding its time slice. */
        static constexpr UInt32 GMaxRelaxSpins = 64U;

        FAtomicFlag Flag;
    };

} // namespace HAL

} // namespace Lumen

    #include "Inline/WindowsSpinLockMutex.inl"

#endif
