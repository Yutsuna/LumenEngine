/**
 * @file WindowsSharedMutex.hpp
 * @brief Readers-writer mutex backed by a Win32 SRWLOCK.
 */

#pragma once

#include <Core/CoreDefinitions.hpp>

#if not defined( LUMEN_PLATFORM_WINDOWS )
    #if not defined( LUMEN_CLANG_TIDY )
        #error "WindowsSharedMutex.hpp is only supported on Windows"
    #endif
#else

    #include <Core/CoreTypes.hpp>
    #include <Core/NonCopyable.hpp>
    #include <Core/NonMovable.hpp>

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
     * @class FWindowsSharedMutex
     * @brief Non-recursive readers-writer mutex.
     * @note  Not recursive: never take the shared lock twice on one thread.
     */
    class CORE_EXPORT FWindowsSharedMutex final : public FNonCopyable, public FNonMovable
    {
    public:

        FWindowsSharedMutex () noexcept  = default;
        ~FWindowsSharedMutex () noexcept = default;

    public:

        LUMEN_FORCE_INLINE void Lock () noexcept;
        LUMEN_FORCE_INLINE void Unlock () noexcept;
        LUMEN_FORCE_INLINE void LockShared () noexcept;
        LUMEN_FORCE_INLINE void UnlockShared () noexcept;

        [[nodiscard]] LUMEN_FORCE_INLINE Bool TryLock () noexcept;
        [[nodiscard]] LUMEN_FORCE_INLINE Bool TryLockShared () noexcept;

    private:

        SRWLOCK Mutex = SRWLOCK_INIT;
    };

} // namespace HAL

} // namespace Lumen

    #include "Inline/WindowsSharedMutex.inl"

#endif
