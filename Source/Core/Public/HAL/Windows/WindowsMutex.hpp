/**
 * @file WindowsMutex.hpp
 * @brief Non-recursive mutex backed by a Win32 SRWLOCK.
 */

#pragma once

#include <Core/CoreDefinitions.hpp>

#if not defined( LUMEN_PLATFROM_WINDOWS )
    #error "WindowsMutex.hpp is only available on Windows platforms."
#else

    #include <Core/CoreTypes.hpp>
    #include <Core/NonCopyable.hpp>
    #include <Core/NonMovable.hpp>

    #ifndef WIN32_LEAN_AND_MEAN
        #define WIN32_LEAN_AND_MEAN
    #endif
    #ifndef NOMINMAX
        #define NOMINMAX
    #endif
    #include <windows.h>

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

        void Lock () noexcept;
        void Unlock () noexcept;
        [[nodiscard]] Bool TryLock () noexcept;

    private:

        SRWLOCK RWLock = SRWLOCK_INIT;
    };

} // namespace HAL

} // namespace Lumen

    #include "Inline/WindowsMutex.inl"

#endif
