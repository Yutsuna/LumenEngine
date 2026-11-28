/**
 * @file ActorMailboxBoundedTests.cpp
 * @brief Capacity, saturation and allocation contracts of the bounded FMailBox.
 */

#include <Actor/ActorLifecycle.hpp>
#include <Actor/ActorMailbox.hpp>
#include <Actor/ActorMessage.hpp>

#include <Core/CoreDefinitions.hpp>
#include <Core/CoreTypes.hpp>

#include <Templates/Array.hpp>
#include <Templates/Optional.hpp>

#include <Thread/Thread.hpp>

#include <Tests/UnitTest.hpp>

#include <AllocationTracking.hpp>

#include <cstddef>
#include <cstdlib>
#include <new>

#if defined( LUMEN_PLATFORM_WINDOWS )
    #include <malloc.h>
#endif

namespace
{
/** @brief Releases a block with the heap API matching Lumen::TrackedAllocate. */
void FreeTrackedAllocation ( void *const InPointer, const Lumen::USize InAlignment = alignof( std::max_align_t ) ) noexcept
{
#if defined( LUMEN_PLATFORM_WINDOWS )
    // UCRT provides _aligned_malloc instead of std::aligned_alloc. Over-aligned blocks
    // must be released with _aligned_free; default-aligned blocks come from malloc.
    if ( InAlignment > alignof( std::max_align_t ) )
    {
        _aligned_free( InPointer );
        return;
    }
#endif
    std::free( InPointer ); /** NOLINT */
}
} // namespace

Lumen::TAtomic<Lumen::UInt64> Lumen::GTrackedAllocations{ 0ULL };
Lumen::TAtomic<Lumen::Bool> Lumen::bTrackingAllocations{ false };

[[nodiscard]] void *Lumen::TrackedAllocate ( const USize InSize, const USize InAlignment ) noexcept
{
    if ( bTrackingAllocations.load( std::memory_order_relaxed ) )
    {
        GTrackedAllocations.fetch_add( 1ULL, std::memory_order_relaxed );
    }

    if ( InAlignment <= alignof( std::max_align_t ) )
    {
        return std::malloc( InSize );
    }

#if defined( LUMEN_PLATFORM_WINDOWS )
    return _aligned_malloc( InSize, InAlignment );
#else
    const USize AlignedSize = ( ( InSize + InAlignment - 1U ) / InAlignment ) * InAlignment;
    return std::aligned_alloc( InAlignment, AlignedSize );
#endif
}

/** INFO: Test-only global allocation hooks; engine code never replaces the global allocator. */
void *operator new( const std::size_t InSize )
{
    void *const Result = Lumen::TrackedAllocate( InSize, alignof( std::max_align_t ) );
    if ( Result == nullptr ) [[unlikely]]
    {
        std::abort();
    }
    return Result;
}

void *operator new[]( const std::size_t InSize )
{
    return ::operator new( InSize );
}

void *operator new( const std::size_t InSize, const std::align_val_t InAlignment )
{
    void *const Result = Lumen::TrackedAllocate( InSize, static_cast<Lumen::USize>( InAlignment ) );
    if ( Result == nullptr ) [[unlikely]]
    {
        std::abort();
    }
    return Result;
}

void *operator new[]( const std::size_t InSize, const std::align_val_t InAlignment )
{
    return ::operator new( InSize, InAlignment );
}

void *operator new( const std::size_t InSize, const std::nothrow_t & ) noexcept
{
    return Lumen::TrackedAllocate( InSize, alignof( std::max_align_t ) );
}

void *operator new[]( const std::size_t InSize, const std::nothrow_t & ) noexcept
{
    return Lumen::TrackedAllocate( InSize, alignof( std::max_align_t ) );
}

void *operator new( const std::size_t InSize, const std::align_val_t InAlignment, const std::nothrow_t & ) noexcept
{
    return Lumen::TrackedAllocate( InSize, static_cast<Lumen::USize>( InAlignment ) );
}

void *operator new[]( const std::size_t InSize, const std::align_val_t InAlignment, const std::nothrow_t & ) noexcept
{
    return Lumen::TrackedAllocate( InSize, static_cast<Lumen::USize>( InAlignment ) );
}

void operator delete( void *const InPointer ) noexcept
{
    FreeTrackedAllocation( InPointer );
}

void operator delete[]( void *const InPointer ) noexcept
{
    FreeTrackedAllocation( InPointer );
}

void operator delete( void *const InPointer, const std::size_t ) noexcept
{
    FreeTrackedAllocation( InPointer );
}

void operator delete[]( void *const InPointer, const std::size_t ) noexcept
{
    FreeTrackedAllocation( InPointer );
}

void operator delete( void *const InPointer, const std::align_val_t InAlignment ) noexcept
{
    FreeTrackedAllocation( InPointer, static_cast<Lumen::USize>( InAlignment ) );
}

void operator delete[]( void *const InPointer, const std::align_val_t InAlignment ) noexcept
{
    FreeTrackedAllocation( InPointer, static_cast<Lumen::USize>( InAlignment ) );
}

void operator delete( void *const InPointer, const std::size_t, const std::align_val_t InAlignment ) noexcept
{
    FreeTrackedAllocation( InPointer, static_cast<Lumen::USize>( InAlignment ) );
}

void operator delete[]( void *const InPointer, const std::size_t, const std::align_val_t InAlignment ) noexcept
{
    FreeTrackedAllocation( InPointer, static_cast<Lumen::USize>( InAlignment ) );
}

void operator delete( void *const InPointer, const std::nothrow_t & ) noexcept
{
    FreeTrackedAllocation( InPointer );
}

void operator delete[]( void *const InPointer, const std::nothrow_t & ) noexcept
{
    FreeTrackedAllocation( InPointer );
}

void operator delete( void *const InPointer, const std::align_val_t InAlignment, const std::nothrow_t & ) noexcept
{
    FreeTrackedAllocation( InPointer, static_cast<Lumen::USize>( InAlignment ) );
}

void operator delete[]( void *const InPointer, const std::align_val_t InAlignment, const std::nothrow_t & ) noexcept
{
    FreeTrackedAllocation( InPointer, static_cast<Lumen::USize>( InAlignment ) );
}

namespace Lumen
{

namespace
{

    constexpr UInt32 BoundedMessageType = 9100U;

    [[nodiscard]] FMessage MakeBoundedMessage ( const UInt32 InValue ) noexcept
    {
        return FMessage::Make( BoundedMessageType, static_cast<ActorID>( InValue ), static_cast<Int32>( InValue ) );
    }

} // namespace

LUMEN_TEST( FMailBox_Bounded, CapacityAcceptsExactlyNAndRejectsNPlusOne )
{
    FMailBox Mailbox( FMailBoxDesc{ .Capacity = 4ULL } );

    LUMEN_CHECK_EQ( Mailbox.GetCapacity(), 4ULL );
    LUMEN_CHECK_TRUE( Mailbox.IsEmpty() );

    for ( UInt32 Index = 0U; Index < 4U; ++Index )
    {
        LUMEN_CHECK_HAS_VALUE( Mailbox.TryPush( MakeBoundedMessage( Index ) ) );
    }

    LUMEN_CHECK_EQ( Mailbox.GetPendingCount(), 4ULL );

    const FMessage Rejected                       = MakeBoundedMessage( 99U );
    const TExpected<void, EActorSendError> Result = Mailbox.TryPush( Rejected );

    LUMEN_CHECK_HAS_ERROR( Result );
    LUMEN_CHECK_EQ( Result.Error(), EActorSendError::MailboxFull );
    LUMEN_CHECK_EQ( Rejected.Sender, 99ULL );
    LUMEN_CHECK_EQ( Mailbox.GetPendingCount(), 4ULL );
    LUMEN_CHECK_EQ( Mailbox.GetSaturationCount(), 1ULL );

    for ( UInt32 Index = 0U; Index < 4U; ++Index )
    {
        const TOptional<FMessage> Item = Mailbox.Pop();
        LUMEN_CHECK_HAS_VALUE( Item );
        LUMEN_CHECK_EQ( Item->Type, BoundedMessageType );
        LUMEN_CHECK_EQ( Item->GetPayload<Int32>(), static_cast<Int32>( Index ) );
    }

    LUMEN_CHECK_TRUE( Mailbox.IsEmpty() );
    LUMEN_CHECK_EQ( Mailbox.GetPendingCount(), 0ULL );
}

LUMEN_TEST( FMailBox_Bounded, RejectionsPreserveAcceptedMessagesAndCountEachFailure )
{
    FMailBox Mailbox( FMailBoxDesc{ .Capacity = 3ULL } );

    for ( UInt32 Index = 0U; Index < 3U; ++Index )
    {
        LUMEN_CHECK_HAS_VALUE( Mailbox.TryPush( MakeBoundedMessage( Index ) ) );
    }

    LUMEN_CHECK_FALSE( Mailbox.Push( MakeBoundedMessage( 10U ) ) );
    LUMEN_CHECK_HAS_ERROR( Mailbox.TryPush( MakeBoundedMessage( 11U ) ) );
    LUMEN_CHECK_EQ( Mailbox.GetSaturationCount(), 2ULL );
    LUMEN_CHECK_EQ( Mailbox.GetPendingCount(), 3ULL );

    for ( UInt32 Index = 0U; Index < 3U; ++Index )
    {
        const TOptional<FMessage> Item = Mailbox.Pop();
        LUMEN_CHECK_HAS_VALUE( Item );
        LUMEN_CHECK_EQ( Item->GetPayload<Int32>(), static_cast<Int32>( Index ) );
    }

    LUMEN_CHECK_TRUE( Mailbox.IsEmpty() );
}

LUMEN_TEST( FMailBox_Bounded, FenceSeparatesAcceptedBatchesWithinOneCapacity )
{
    FMailBox Mailbox( FMailBoxDesc{ .Capacity = 4ULL } );

    for ( UInt32 Index = 0U; Index < 2U; ++Index )
    {
        LUMEN_CHECK_HAS_VALUE( Mailbox.TryPush( MakeBoundedMessage( Index ) ) );
    }

    Mailbox.BeginTick();

    for ( UInt32 Index = 2U; Index < 4U; ++Index )
    {
        LUMEN_CHECK_HAS_VALUE( Mailbox.TryPush( MakeBoundedMessage( Index ) ) );
    }

    LUMEN_CHECK_EQ( Mailbox.GetPendingCount(), 4ULL );

    for ( UInt32 Index = 0U; Index < 2U; ++Index )
    {
        LUMEN_CHECK_EQ( Mailbox.Pop()->GetPayload<Int32>(), static_cast<Int32>( Index ) );
    }
    LUMEN_CHECK_FALSE( Mailbox.Pop().HasValue() );

    Mailbox.BeginTick();

    for ( UInt32 Index = 2U; Index < 4U; ++Index )
    {
        LUMEN_CHECK_EQ( Mailbox.Pop()->GetPayload<Int32>(), static_cast<Int32>( Index ) );
    }
    LUMEN_CHECK_FALSE( Mailbox.Pop().HasValue() );

    Mailbox.EndTick();
}

LUMEN_TEST( FMailBox_Bounded, ReserveGrowsAndTransfersPendingMessages )
{
    FMailBox Mailbox( FMailBoxDesc{ .Capacity = 2ULL } );

    LUMEN_CHECK_HAS_VALUE( Mailbox.TryPush( MakeBoundedMessage( 1U ) ) );
    LUMEN_CHECK_HAS_VALUE( Mailbox.TryPush( MakeBoundedMessage( 2U ) ) );
    LUMEN_CHECK_HAS_ERROR( Mailbox.TryPush( MakeBoundedMessage( 3U ) ) );

    LUMEN_CHECK_HAS_VALUE( Mailbox.Reserve( 8ULL ) );
    LUMEN_CHECK_EQ( Mailbox.GetCapacity(), 8ULL );
    LUMEN_CHECK_EQ( Mailbox.GetPendingCount(), 2ULL );

    for ( UInt32 Index = 1U; Index <= 2U; ++Index )
    {
        LUMEN_CHECK_EQ( Mailbox.Pop()->GetPayload<Int32>(), static_cast<Int32>( Index ) );
    }

    LUMEN_CHECK_HAS_VALUE( Mailbox.TryPush( MakeBoundedMessage( 3U ) ) );
    LUMEN_CHECK_EQ( Mailbox.Pop()->GetPayload<Int32>(), 3 );

    /** A smaller or equal reserve is an idempotent success; an invalid descriptor leaves a mailbox unusable. */
    LUMEN_CHECK_HAS_VALUE( Mailbox.Reserve( 2ULL ) );
    LUMEN_CHECK_EQ( Mailbox.GetCapacity(), 8ULL );

    FMailBox Invalid( FMailBoxDesc{ .Capacity = 0ULL } );
    LUMEN_CHECK_EQ( Invalid.GetCapacity(), 0ULL );
    LUMEN_CHECK_HAS_ERROR( Invalid.TryPush( MakeBoundedMessage( 1U ) ) );
    LUMEN_CHECK_EQ( Invalid.GetSaturationCount(), 1ULL );
}

LUMEN_TEST( FMailBox_Bounded, ConcurrentProducersRespectExactCapacity )
{
    constexpr UInt32 ProducerCount = 4U;

    FMailBox Mailbox( FMailBoxDesc{ .Capacity = static_cast<Lumen::USize>( ProducerCount ) } );
    TAtomic<UInt32> Accepted = { 0U };
    TAtomic<UInt32> Rejected = { 0U };

    TArray<FThread, ProducerCount> Producers = {
        FThread(
            [&Mailbox, &Accepted, &Rejected] ()
    {
        if ( Mailbox.TryPush( MakeBoundedMessage( 1U ) ).HasValue() )
        {
            Accepted.fetch_add( 1U, std::memory_order_relaxed );
        }
        else
        {
            Rejected.fetch_add( 1U, std::memory_order_relaxed );
        }
    } ),
        FThread(
            [&Mailbox, &Accepted, &Rejected] ()
    {
        if ( Mailbox.TryPush( MakeBoundedMessage( 2U ) ).HasValue() )
        {
            Accepted.fetch_add( 1U, std::memory_order_relaxed );
        }
        else
        {
            Rejected.fetch_add( 1U, std::memory_order_relaxed );
        }
    } ),
        FThread(
            [&Mailbox, &Accepted, &Rejected] ()
    {
        if ( Mailbox.TryPush( MakeBoundedMessage( 3U ) ).HasValue() )
        {
            Accepted.fetch_add( 1U, std::memory_order_relaxed );
        }
        else
        {
            Rejected.fetch_add( 1U, std::memory_order_relaxed );
        }
    } ),
        FThread(
            [&Mailbox, &Accepted, &Rejected] ()
    {
        if ( Mailbox.TryPush( MakeBoundedMessage( 4U ) ).HasValue() )
        {
            Accepted.fetch_add( 1U, std::memory_order_relaxed );
        }
        else
        {
            Rejected.fetch_add( 1U, std::memory_order_relaxed );
        }
    } ),
    };

    for ( FThread &Producer : Producers )
    {
        Producer.Join();
    }

    LUMEN_CHECK_EQ( Accepted.load( std::memory_order_relaxed ), ProducerCount );
    LUMEN_CHECK_EQ( Rejected.load( std::memory_order_relaxed ), 0U );
    LUMEN_CHECK_EQ( Mailbox.GetSaturationCount(), 0ULL );

    UInt32 ReceivedCount = 0U;
    for ( UInt32 Index = 0U; Index < ProducerCount; ++Index )
    {
        if ( Mailbox.Pop().HasValue() )
        {
            ++ReceivedCount;
        }
    }
    LUMEN_CHECK_EQ( ReceivedCount, ProducerCount );
    LUMEN_CHECK_TRUE( Mailbox.IsEmpty() );
}

LUMEN_TEST( FMailBox_Bounded, OneMillionPushPopAfterReservePerformsNoAllocation )
{
    constexpr UInt32 IterationCount = 1000000U;

    FMailBox Mailbox( FMailBoxDesc{ .Capacity = 1ULL } );
    const FMessage Message = MakeBoundedMessage( 7U );

    Bool bAllAccepted      = true;
    UInt64 ReceivedCount   = 0ULL;
    UInt64 AllocationCount = 0ULL;

    {
        const FAllocationTrackingScope Tracking;
        for ( UInt32 Index = 0U; Index < IterationCount; ++Index )
        {
            bAllAccepted = bAllAccepted and Mailbox.TryPush( Message ).HasValue();
            ReceivedCount += Mailbox.Pop().HasValue() ? 1ULL : 0ULL;
        }
        AllocationCount = Tracking.GetCount();
    }

    /** INFO: The 1M steady-state exchanges must not touch the global allocator. */
    LUMEN_CHECK_EQ( AllocationCount, 0ULL );
    LUMEN_CHECK_TRUE( bAllAccepted );
    LUMEN_CHECK_EQ( ReceivedCount, static_cast<UInt64>( IterationCount ) );
    LUMEN_CHECK_EQ( Mailbox.GetSaturationCount(), 0ULL );
    LUMEN_CHECK_TRUE( Mailbox.IsEmpty() );
}

} // namespace Lumen
