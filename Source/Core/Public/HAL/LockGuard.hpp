/**
 * @file LockGuards.hpp
 * @brief Generic RAII lock wrapper usable with any FPlatform*Mutex.
 */

#pragma once

#include <Core/CoreDefinitions.hpp>
#include <Core/CoreTypes.hpp>
#include <Core/NonCopyable.hpp>
#include <Core/NonMovable.hpp>

namespace Lumen
{

namespace HAL
{

    /**
     * @trait FDeferLockT
     * @brief Trait used to indicate a deferred lock should be used.
     */
    inline constexpr struct CORE_EXPORT FDeferLock
    {
        explicit constexpr FDeferLock () = default;
    } FDeferLockT;

    /**
     * @trait FTryToLock
     * @brief Trait used to indicate a try-to-lock should be used.
     */
    inline constexpr struct CORE_EXPORT FTryToLock
    {
        explicit constexpr FTryToLock () = default;
    } FTryToLockT;

    /**
     * @trait FAdoptLock
     * @brief Trait used to indicate an adopt lock should be used.
     */
    inline constexpr struct CORE_EXPORT FAdoptLock
    {
        explicit constexpr FAdoptLock () = default;
    } FAdoptLockT;

    /**
     * @class TLockGuard
     * @brief Scope guard Lock() on construction and Unlock on destruction.
     * @tparam TMutexType A Type exposing Lock() and Unlock().
     */
    template <class TMutexType> class [[nodiscard]] TLockGuard final : public FNonCopyable, public FNonMovable
    {
    public:

        LUMEN_FORCE_INLINE explicit TLockGuard( TMutexType &InMutex ) noexcept;
        LUMEN_FORCE_INLINE ~TLockGuard() noexcept;

    private:

        TMutexType &Mutex;
    };

    /**
     * @class TSharedLock
     * @brief Scope guard LockShared() on construction and UnlockShared on destruction.
     * @tparam TMutexType A Type exposing LockShared() and UnlockShared().
     */
    template <class TMutexType> class [[nodiscard]] TSharedLock final : public FNonCopyable, public FNonMovable
    {
    public:

        LUMEN_FORCE_INLINE explicit TSharedLock( TMutexType &InMutex ) noexcept;
        LUMEN_FORCE_INLINE ~TSharedLock() noexcept;

    private:

        TMutexType &Mutex;
    };

    /**
     * @class TUniqueLock
     * @brief Move-only mutex ownership wrapper.
     * @tparam TMutexType A Type exposing Lock(), TryLock() and Unlock().
     */
    template <class TMutexType> class [[nodiscard]] TUniqueLock final : public FNonCopyable, public FNonMovable
    {
    public:

        TUniqueLock () noexcept = default;

        explicit TUniqueLock ( TMutexType &InMutex ) noexcept;

        TUniqueLock ( TMutexType &InMutex, FDeferLock ) noexcept;
        TUniqueLock ( TMutexType &InMutex, FTryToLock ) noexcept;
        TUniqueLock ( TMutexType &InMutex, FAdoptLock ) noexcept;

        ~TUniqueLock () noexcept;

    public:

        TUniqueLock ( TUniqueLock &&InOther ) noexcept;
        TUniqueLock &operator=( TUniqueLock &&InOther ) noexcept;

    public:

        void Lock () noexcept;
        void Unlock () noexcept;

        [[nodiscard]] Bool TryLock () noexcept;
        [[nodiscard]] Bool IsLocked () const noexcept;
        [[nodiscard]] explicit operator Bool () const noexcept;

        TMutexType *Release () noexcept;

    private:

        TMutexType *Mutex = nullptr;
        Bool bIsLocked    = false;
    };

    /** @brief Write-side guard of a shared mutex. */
    template <typename MutexType> using TSharedUniqueLock = TUniqueLock<MutexType>;

} // namespace HAL

} // namespace Lumen

#include "Inline/LockGuard.inl"
