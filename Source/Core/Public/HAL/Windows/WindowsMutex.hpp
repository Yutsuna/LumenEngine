/**
 * @file WindowsMutex.hpp
 * @brief Non-recursive mutex backed by a Win32 SRWLOCK.
 */

#pragma once

#include <Core/CoreDefinitions.hpp>

#if not defined( LUMEN_PLATFORM_WINDOWS )
    #if not defined( LUMEN_CLANG_TIDY )
        #error "WindowsMutex.hpp is only supported on Windows"
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
     * @class FWindowsMutex
     * @brief Non-recursive, pointer-sized mutex.
     */
    class CORE_EXPORT FWindowsMutex final : public FNonCopyable, public FNonMovable
    {
    public:

        FWindowsMutex () noexcept  = default;
        ~FWindowsMutex () noexcept = default;

    public:

        LUMEN_FORCE_INLINE void Lock () noexcept;
        LUMEN_FORCE_INLINE void Unlock () noexcept;
        [[nodiscard]] LUMEN_FORCE_INLINE Bool TryLock () noexcept;

    private:

        SRWLOCK RWLock = SRWLOCK_INIT;
    };

} // namespace HAL

} // namespace Lumen

    #include "Inline/WindowsMutex.inl"

#endif
