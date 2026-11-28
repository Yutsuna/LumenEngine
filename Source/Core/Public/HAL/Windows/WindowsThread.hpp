/**
 * @file WindowsThread.hpp
 * @brief Win32 thread ownership and operations.
 */

#pragma once

#include <Core/CoreDefinitions.hpp>

#if not defined( LUMEN_PLATFORM_WINDOWS )
    #if not defined( LUMEN_CLANG_TIDY )
        #error "WindowsThread.hpp is only supported on Windows"
    #endif
#else

    #include <Core/CoreTypes.hpp>
    #include <Core/NonCopyable.hpp>
    #include <CoreExport.hpp>

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

/* winbase.h defines Yield() as an empty legacy macro, which collides with FWindowsThread::Yield(). */
    #undef Yield

/* winbase.h defines CreateSemaphore as CreateSemaphoreA/W, which collides with
 * IRHIDevice::CreateSemaphore and its overrides. No Core HAL code calls the Win32
 * function, so drop the legacy macro before any engine declaration. */
    #undef CreateSemaphore

namespace Lumen
{
namespace HAL
{
    class CORE_EXPORT FWindowsThread final : public FNonCopyable
    {
    public:

        /** MinGW does not propagate the enclosing class dllexport to nested types. */
        struct CORE_EXPORT FId
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
