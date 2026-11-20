/**
 * @file VulkanHandleTests.cpp
 * @brief Functional tests for the Vulkan scoped handle wrapper.
 */

#include <Tests/UnitTest.hpp>
#include <RHIVulkan/Types/VulkanHandle.hpp>

namespace Lumen
{

namespace RHIVulkan
{

    namespace
    {

        struct FTestDeletionState
        {
            USize CallCount         = 0U;
            void *LastDeletedHandle = nullptr;
        };

        struct FTestDeleter
        {
            FTestDeletionState *State = nullptr;

            void operator()( void *InHandle ) const noexcept
            {
                ++State->CallCount;
                State->LastDeletedHandle = InHandle;
            }
        };

        using FTestHandle = TVulkanScopedHandle<void *, FTestDeleter>;

        [[nodiscard]] void *MakeTestHandle ( const UInt64 InValue ) noexcept
        {
            return reinterpret_cast<void *>( InValue );
        }

    } // namespace

    LUMEN_TEST( VulkanHandle, DefaultConstructedIsInvalid )
    {
        const FTestHandle Handle;
        LUMEN_CHECK_FALSE( Handle.IsValid() );
        LUMEN_CHECK_EQ( Handle.Get(), nullptr );
        LUMEN_CHECK_FALSE( static_cast<Bool>( Handle ) );
    }

    LUMEN_TEST( VulkanHandle, ConstructedWithValueIsValid )
    {
        FTestDeletionState State;
        void *const RawHandle = MakeTestHandle( 42ULL );
        const FTestHandle Handle( RawHandle, FTestDeleter{ &State } );
        LUMEN_CHECK_TRUE( Handle.IsValid() );
        LUMEN_CHECK_EQ( Handle.Get(), RawHandle );
        LUMEN_CHECK_TRUE( static_cast<Bool>( Handle ) );
    }

    LUMEN_TEST( VulkanHandle, DestructorInvokesDeleter )
    {
        FTestDeletionState State;
        {
            const FTestHandle Handle( MakeTestHandle( 7ULL ), FTestDeleter{ &State } );
            LUMEN_CHECK_EQ( Handle.GetDeleter().State->CallCount, 0U );
        }
        LUMEN_CHECK_EQ( State.CallCount, 1U );
        LUMEN_CHECK_EQ( State.LastDeletedHandle, MakeTestHandle( 7ULL ) );
    }

    LUMEN_TEST( VulkanHandle, NullHandleDoesNotInvokeDeleter )
    {
        FTestDeletionState State;
        {
            const FTestHandle Handle( nullptr, FTestDeleter{ &State } );
        }
        LUMEN_CHECK_EQ( State.CallCount, 0U );
    }

    LUMEN_TEST( VulkanHandle, ReleaseReturnsHandleAndInvalidatesOwner )
    {
        FTestDeletionState State;
        void *const RawHandle = MakeTestHandle( 99ULL );
        FTestHandle Handle( RawHandle, FTestDeleter{ &State } );
        LUMEN_CHECK_TRUE( Handle.IsValid() );
        LUMEN_CHECK_EQ( Handle.Release(), RawHandle );
        LUMEN_CHECK_FALSE( Handle.IsValid() );
    }

    LUMEN_TEST( VulkanHandle, MoveConstructionTransfersOwnership )
    {
        FTestDeletionState State;
        FTestHandle Source( MakeTestHandle( 5ULL ), FTestDeleter{ &State } );
        FTestHandle Destination( static_cast<FTestHandle &&>( Source ) );
        LUMEN_CHECK_FALSE( Source.IsValid() );
        LUMEN_CHECK_TRUE( Destination.IsValid() );
        LUMEN_CHECK_EQ( Destination.Get(), MakeTestHandle( 5ULL ) );
    }

    LUMEN_TEST( VulkanHandle, MoveAssignmentTransfersOwnership )
    {
        FTestDeletionState State;
        FTestHandle Source( MakeTestHandle( 11ULL ), FTestDeleter{ &State } );
        FTestHandle Destination( MakeTestHandle( 3ULL ), FTestDeleter{ &State } );
        Destination = static_cast<FTestHandle &&>( Source );
        LUMEN_CHECK_FALSE( Source.IsValid() );
        LUMEN_CHECK_TRUE( Destination.IsValid() );
        LUMEN_CHECK_EQ( Destination.Get(), MakeTestHandle( 11ULL ) );
        LUMEN_CHECK_EQ( State.CallCount, 1U );
    }

    LUMEN_TEST( VulkanHandle, SelfAssignmentIsNoOp )
    {
        FTestDeletionState State;
        FTestHandle Handle( MakeTestHandle( 13ULL ), FTestDeleter{ &State } );
        Handle = static_cast<FTestHandle &&>( Handle );
        LUMEN_CHECK_TRUE( Handle.IsValid() );
        LUMEN_CHECK_EQ( Handle.Get(), MakeTestHandle( 13ULL ) );
        LUMEN_CHECK_EQ( State.CallCount, 0U );
    }

    LUMEN_TEST( VulkanHandle, ResetDestroysOldHandleAndAdoptsNew )
    {
        FTestDeletionState State;
        FTestHandle Handle( MakeTestHandle( 17ULL ), FTestDeleter{ &State } );
        Handle.Reset( MakeTestHandle( 19ULL ) );
        LUMEN_CHECK_EQ( State.CallCount, 1U );
        LUMEN_CHECK_EQ( Handle.Get(), MakeTestHandle( 19ULL ) );
        LUMEN_CHECK_TRUE( Handle.IsValid() );
    }

    LUMEN_TEST( VulkanHandle, ResetWithSameHandleDoesNotDestroy )
    {
        void *const RawHandle = MakeTestHandle( 17ULL );
        FTestDeletionState State;
        FTestHandle Handle( RawHandle, FTestDeleter{ &State } );
        Handle.Reset( RawHandle );
        LUMEN_CHECK_EQ( State.CallCount, 0U );
    }

    LUMEN_TEST( VulkanHandle, ResetAndGetAddressOfYieldsWritablePointer )
    {
        FTestDeletionState State;
        FTestHandle Handle( MakeTestHandle( 1ULL ), FTestDeleter{ &State } );
        void **const Address = Handle.ResetAndGetAddressOf( FTestDeleter{ &State } );
        LUMEN_CHECK_TRUE( Address != nullptr );
        LUMEN_CHECK_EQ( *Address, nullptr );
        *Address = MakeTestHandle( 23ULL );
        LUMEN_CHECK_EQ( Handle.Get(), MakeTestHandle( 23ULL ) );
        LUMEN_CHECK_TRUE( Handle.IsValid() );
    }

} // namespace RHIVulkan

} // namespace Lumen
