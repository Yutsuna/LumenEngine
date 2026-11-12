/**
 * @file UnixSpinLockMutex.hpp
 * @brief Busy-wait spin lock for POSIX UNIX platforms.
 */

#pragma once

#include <Core/CoreDefinitions.hpp>

#if not defined( LUMEN_PLATFORM_UNIX )
    #error "FUnixSpinLockMutex.hpp is only available on Unix platforms."
#else

    #include <Core/CoreTypes.hpp>
    #include <Core/NonCopyable.hpp>
    #include <Core/NonMovable.hpp>

namespace Lumen
{

namespace HAL
{

    class CORE_EXPORT FUnixSpinLockMutex final : public FNonCopyable, public FNonMovable
    {
    public:

        FUnixSpinLockMutex () noexcept  = default;
        ~FUnixSpinLockMutex () noexcept = default;

    public:

        LUMEN_FORCE_INLINE void Lock () noexcept;
        LUMEN_FORCE_INLINE void Unlock () noexcept;

        [[nodiscard]] LUMEN_FORCE_INLINE Bool TryLock () noexcept;
        [[nodiscard]] LUMEN_FORCE_INLINE Bool IsLocked () const noexcept;

    private:

        /** @brief Number of CpuRelax() iterations before the waiter starts yielding its time slice. */
        static constexpr UInt32 GMaxRelaxSpins = 64U;

        FAtomicFlag Flag{};
    };

    using FPlatformSpinLockMutex = FUnixSpinLockMutex;

} // namespace HAL

} // namespace Lumen

    #include "Inline/UnixSpinLock.inl"

#endif
