/**
 * @file UnixMutex.hpp
 * @brief Non-recursive mutex backed by a POSIX pthread mutex.
 */

#pragma once

#include <Core/CoreDefinitions.hpp>

#if not defined( LUMEN_PLATFORM_UNIX )
    #error "UnixRecursiveMutex.hpp is only available on Unix platforms."
#else

    #include <Core/CoreTypes.hpp>
    #include <Core/NonCopyable.hpp>
    #include <Core/NonMovable.hpp>

    #include <pthread.h>

namespace Lumen
{

namespace HAL
{

    /**
     * @class FUnixMutex
     * @brief Non-recursive, OS-parking mutex. Safe to use at global.
     * @warning Not async-signal-safe: never lock it from an OS signal handler.
     */
    class CORE_EXPORT FUnixMutex final : public FNonCopyable, public FNonMovable
    {
    public:

        FUnixMutex () noexcept = default;
        ~FUnixMutex () noexcept;

    public:

        void Lock () noexcept;
        void Unlock () noexcept;

        [[nodiscard]] Bool TryLock () noexcept;

    private:

        pthread_mutex_t Mutex = PTHREAD_MUTEX_INITIALIZER;
    };

    using FPlatformMutex = FUnixMutex;

} // namespace HAL

} // namespace Lumen

    #include "Inline/UnixMutex.inl"

#endif
