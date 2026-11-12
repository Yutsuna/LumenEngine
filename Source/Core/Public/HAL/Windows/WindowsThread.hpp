/**
 * @file WindowsThread.hpp
 * @brief Win32 thread ownership and operations.
 */

#pragma once

#include <Core/CoreDefinitions.hpp>

#if not defined( LUMEN_PLATFORM_WINDOWS )
    #error "WindowsThread.hpp is only available on Windows platforms."
#else

    #include <Core/CoreTypes.hpp>
    #include <Core/NonCopyable.hpp>
    #include <CoreExport.hpp>

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
    class CORE_EXPORT FWindowsThread final : public FNonCopyable
    {
    public:

        struct FId
        {
            FId () noexcept = default;
            explicit FId ( DWORD InId ) noexcept;

            [[nodiscard]] Bool IsValid () const noexcept;
            [[nodiscard]] Bool operator==( const FId &InOther ) const noexcept;

        private:

            DWORD Value = 0;
        };

        using FNativeHandle = HANDLE;
        using FEntryPoint   = DWORD( WINAPI * )( void * );

        FWindowsThread () noexcept = default;
        FWindowsThread ( FWindowsThread &&InOther ) noexcept;
        FWindowsThread &operator=( FWindowsThread &&InOther ) noexcept;
        ~FWindowsThread () noexcept;

        [[nodiscard]] Bool Start ( FEntryPoint InEntryPoint, void *InData ) noexcept;
        void Join () noexcept;
        void Detach () noexcept;
        void Swap ( FWindowsThread &InOther ) noexcept;

        [[nodiscard]] Bool IsJoinable () const noexcept;
        [[nodiscard]] FId GetId () const noexcept;
        [[nodiscard]] FNativeHandle GetNativeHandle () const noexcept;

        [[nodiscard]] static FId GetCurrentId () noexcept;
        [[nodiscard]] static UInt32 HardwareConcurrency () noexcept;
        static void Yield () noexcept;

    private:

        void Reset () noexcept;

    private:

        HANDLE Thread  = nullptr;
        DWORD ThreadId = 0;
    };
} // namespace HAL
} // namespace Lumen

#endif
