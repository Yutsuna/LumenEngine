/**
 * @file WindowsRecursiveSharedMutex.hpp
 * @brief Recursive mutex backed by a Win32 CRITICAL_SECTION.
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
     * @class FWindowsRecursiveSharedMutex
     * @brief Recursive mutex with a spin count.
     */
    class CORE_EXPORT FWindowsRecursiveSharedMutex final : public FNonCopyable, public FNonMovable
    {
    public:

        LUMEN_FORCE_INLINE FWindowsRecursiveSharedMutex () noexcept;
        LUMEN_FORCE_INLINE ~FWindowsRecursiveSharedMutex() noexcept;

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

    #include "Inline/WindowsRecursiveSharedMutex.inl"

#endif
