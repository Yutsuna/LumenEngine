/**
 * @file WindowsRecursiveMutex.hpp
 * @brief Recursive mutex backed by a Win32 CRITICAL_SECTION.
 */

#pragma once

#include <Core/CoreDefinitions.hpp>

#if not defined( LUMEN_PLATFROM_WINDOWS )
    #error "WindowsMutex.hpp is only available on Unix platforms."
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
     * @class FWindowsRecursiveMutex
     * @brief Recursive mutex with a spin count.
     */
    class CORE_EXPORT FWindowsRecursiveMutex final : public FNonCopyable, public FNonMovable
    {
    public:

        LUMEN_FORCE_INLINE FWindowsRecursiveMutex () noexcept;
        LUMEN_FORCE_INLINE ~FWindowsRecursiveMutex() noexcept;

    public:

        LUMEN_FORCE_INLINE void Lock () noexcept;
        LUMEN_FORCE_INLINE void Unlock () noexcept;
        LUMEN_FORCE_INLINE [[nodiscard]] Bool TryLock () noexcept;

    private:

        /** @brief Spin iterations before blocking. */
        static constexpr DWORD GSpinCount = 4000U;

        CRITICAL_SECTION Mutex;
    };

} // namespace HAL

} // namespace Lumen

    #include "Inline/WindowsRecursiveMutex.inl"

#endif
