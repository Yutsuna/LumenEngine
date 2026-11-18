/**
 * @file WindowsSharedMutex.hpp
 * @brief Readers-writer mutex backed by a Win32 SRWLOCK.
 */

#pragma once

#include <Core/CoreDefinitions.hpp>

#if not defined( LUMEN_PLATFROM_WINDOWS )
    #error "WindowsMutex.hpp is only available on Windows platforms."
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
