
#include <Concepts/IsLockable.hpp>
#include <Tests/UnitTest.hpp>

#include <HAL/PlatformMutex.hpp>

namespace Lumen
{

namespace
{

    /**
     * Data Tests
     */

    /** @brief Minimal lockable type exposing both required operations. */
    struct FCustomLockable
    {
        void Lock () noexcept
        {
            /* */
        }

        void Unlock () noexcept
        {
            /* */
        }
    };

    /** @brief Near-miss exposing only Lock(). */
    struct FLockOnly
    {
        void Lock () noexcept
        {
            /* */
        }
    };

    /** @brief Near-miss exposing only Unlock(). */
    struct FUnlockOnly
    {
        void Unlock () noexcept
        {
            /* */
        }
    };

    struct FNotLockable
    {
        /* */
    };

    /**
     * Tests
     */

    LUMEN_TEST( Concepts_IsLockable, TIsLockable )
    {
        LUMEN_CHECK_TRUE( ( Concepts::CIsLockable<FCustomLockable> ));
        LUMEN_CHECK_TRUE( ( Concepts::CIsLockable<FCustomLockable &> ));
        LUMEN_CHECK_TRUE( ( Concepts::CIsLockable<HAL::FPlatformMutex> ));
        LUMEN_CHECK_TRUE( ( Concepts::CIsLockable<HAL::FPlatformSharedMutex> ));
        LUMEN_CHECK_TRUE( ( Concepts::CIsLockable<HAL::FPlatformRecursiveSharedMutex> ));
        LUMEN_CHECK_TRUE( ( Concepts::CIsLockable<HAL::FPlatformSpinLockMutex> ));

        LUMEN_CHECK_TRUE( ( not Concepts::CIsLockable<FLockOnly> ));
        LUMEN_CHECK_TRUE( ( not Concepts::CIsLockable<FUnlockOnly> ));
        LUMEN_CHECK_TRUE( ( not Concepts::CIsLockable<FNotLockable> ));
        LUMEN_CHECK_TRUE( ( not Concepts::CIsLockable<const FCustomLockable> ));
        LUMEN_CHECK_TRUE( ( not Concepts::CIsLockable<FCustomLockable *> ));
        LUMEN_CHECK_TRUE( ( not Concepts::CIsLockable<Int32> ));
        LUMEN_CHECK_TRUE( ( not Concepts::CIsLockable<NullPtrType> ));
    }

} // namespace

} // namespace Lumen
