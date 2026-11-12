/**
 * @file UnixThread.hpp
 * @brief POSIX thread ownership and operations.
 */

#pragma once

#include <Core/CoreDefinitions.hpp>

#if not defined( LUMEN_PLATFORM_UNIX )
    #error "UnixThread.hpp is only available on Unix platforms."
#else

    #include <Core/CoreTypes.hpp>
    #include <Core/NonCopyable.hpp>
    #include <CoreExport.hpp>

    #include <pthread.h>

namespace Lumen
{
namespace HAL
{
    class CORE_EXPORT FUnixThread final : public FNonCopyable
    {
    public:

        struct FId
        {
            FId () noexcept = default;
            explicit FId ( pthread_t InId ) noexcept;

            [[nodiscard]] Bool IsValid () const noexcept;
            [[nodiscard]] Bool operator==( const FId &InOther ) const noexcept;

        private:

            pthread_t Value{};
            Bool bValid = false;
        };

        using FNativeHandle = pthread_t;
        using FEntryPoint   = void *( * )( void * );

        FUnixThread () noexcept = default;
        FUnixThread ( FUnixThread &&InOther ) noexcept;
        FUnixThread &operator=( FUnixThread &&InOther ) noexcept;
        ~FUnixThread () noexcept;

        [[nodiscard]] Bool Start ( FEntryPoint InEntryPoint, void *InData ) noexcept;
        void Join () noexcept;
        void Detach () noexcept;
        void Swap ( FUnixThread &InOther ) noexcept;

        [[nodiscard]] Bool IsJoinable () const noexcept;
        [[nodiscard]] FId GetId () const noexcept;
        [[nodiscard]] FNativeHandle GetNativeHandle () const noexcept;

        [[nodiscard]] static FId GetCurrentId () noexcept;
        [[nodiscard]] static UInt32 HardwareConcurrency () noexcept;
        static void Yield () noexcept;

    private:

        void Reset () noexcept;

    private:

        pthread_t Thread{};
        Bool bJoinable = false;
    };
} // namespace HAL
} // namespace Lumen

#endif
