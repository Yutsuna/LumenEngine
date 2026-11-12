/**
 * @file UnixSharedMutex.hpp
 * @brief Readers-writer mutex backed by a POSIX pthread rwlock.
 */

#pragma once

#include <Core/CoreDefinitions.hpp>

#if not defined( LUMEN_PLATFORM_UNIX )
    #error "UnixSharedMutex.hpp is only available on Unix platforms."
#else

    #include <Core/CoreTypes.hpp>
    #include <Core/NonCopyable.hpp>
    #include <Core/NonMovable.hpp>

    #include <pthread.h>

namespace Lumen
{

namespace HAL
{

    class CORE_EXPORT FUnixSharedMutex final : public FNonCopyable, public FNonMovable
    {
    public:

        FUnixSharedMutex () noexcept = default;
        LUMEN_FORCE_INLINE ~FUnixSharedMutex() noexcept;

        LUMEN_FORCE_INLINE void Lock () noexcept;
        LUMEN_FORCE_INLINE void Unlock () noexcept;

        LUMEN_FORCE_INLINE void LockShared () noexcept;
        LUMEN_FORCE_INLINE void UnlockShared () noexcept;

        [[nodiscard]] LUMEN_FORCE_INLINE Bool TryLock () noexcept;
        [[nodiscard]] LUMEN_FORCE_INLINE Bool TryLockShared () noexcept;

    private:

    #if defined( PTHREAD_RWLOCK_WRITER_NONRECURSIVE_INITIALIZER_NP )
        pthread_rwlock_t RwLock = PTHREAD_RWLOCK_WRITER_NONRECURSIVE_INITIALIZER_NP;
    #else
        pthread_rwlock_t RwLock = PTHREAD_RWLOCK_INITIALIZER;
    #endif
    };

    using FPlatformSharedMutex = FUnixSharedMutex;

} // namespace HAL

} // namespace Lumen

    #include "Inline/UnixSharedMutex.inl"

#endif
