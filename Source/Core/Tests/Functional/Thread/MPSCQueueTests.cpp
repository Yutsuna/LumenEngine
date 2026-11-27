/**
 * @file MPSCQueueTests.cpp
 * @brief Functional tests for Lumen::Thread::TMPSCQueue.
 */

#include <Tests/UnitTest.hpp>

#include <Core/CoreTypes.hpp>

#include <Templates/Array.hpp>

#include <Thread/MPSCQueue.hpp>
#include <Thread/Thread.hpp>

#include <limits>
#include <type_traits>
#include <utility>

namespace Lumen
{

namespace
{

    using FIntQueue = Thread::TMPSCQueue<Int32>;

    static_assert( not std::is_copy_constructible_v<FIntQueue> and not std::is_copy_assignable_v<FIntQueue>,
                   "TMPSCQueue must not be copyable" );
    static_assert( not std::is_move_constructible_v<FIntQueue> and not std::is_move_assignable_v<FIntQueue>,
                   "TMPSCQueue must not be movable" );
    static_assert( std::is_nothrow_default_constructible_v<FIntQueue>, "TMPSCQueue default construction must be noexcept" );
    static_assert( std::is_nothrow_destructible_v<FIntQueue>, "TMPSCQueue destruction must be noexcept" );
    static_assert( noexcept( std::declval<FIntQueue &>().Initialize( 1ULL ) ) );
    static_assert( noexcept( std::declval<FIntQueue &>().Push( std::declval<const Int32 &>() ) ) );
    static_assert( noexcept( std::declval<FIntQueue &>().Push( std::declval<Int32 &&>() ) ) );
    static_assert( noexcept( std::declval<FIntQueue &>().Pop() ) );
    static_assert( noexcept( std::declval<const FIntQueue &>().IsInitialized() ) );
    static_assert( noexcept( std::declval<const FIntQueue &>().GetCapacity() ) );
    static_assert( noexcept( std::declval<const FIntQueue &>().IsEmpty() ) );

    struct FPayloadLifetime
    {
        Int32 ConstructionCount = 0;
        Int32 DestructionCount  = 0;
        Int32 LiveCount         = 0;
    };

    struct FTrackedPayload
    {
        FPayloadLifetime *Lifetime;
        Int32 Value;

        FTrackedPayload ( FPayloadLifetime &InLifetime, const Int32 InValue ) noexcept : Lifetime( &InLifetime ), Value( InValue )
        {
            ++Lifetime->ConstructionCount;
            ++Lifetime->LiveCount;
        }

        FTrackedPayload ( const FTrackedPayload &InOther ) noexcept : Lifetime( InOther.Lifetime ), Value( InOther.Value )
        {
            ++Lifetime->ConstructionCount;
            ++Lifetime->LiveCount;
        }

        FTrackedPayload ( FTrackedPayload &&InOther ) noexcept
            : Lifetime( InOther.Lifetime ), Value( std::exchange( InOther.Value, -1 ) )
        {
            ++Lifetime->ConstructionCount;
            ++Lifetime->LiveCount;
        }

        FTrackedPayload &operator=( const FTrackedPayload & ) = delete;
        FTrackedPayload &operator=( FTrackedPayload && )      = delete;

        ~FTrackedPayload () noexcept
        {
            ++Lifetime->DestructionCount;
            --Lifetime->LiveCount;
        }
    };

    static_assert( not std::is_default_constructible_v<FTrackedPayload>, "The queue must not require default payloads" );

    struct FMoveOnlyPayload
    {
        Int32 Value;
        Bool bValid = true;

        explicit FMoveOnlyPayload ( const Int32 InValue ) noexcept : Value( InValue )
        {
        }

        FMoveOnlyPayload ( const FMoveOnlyPayload & )           = delete;
        FMoveOnlyPayload &operator=( const FMoveOnlyPayload & ) = delete;

        FMoveOnlyPayload ( FMoveOnlyPayload &&InOther ) noexcept
            : Value( std::exchange( InOther.Value, -1 ) ), bValid( std::exchange( InOther.bValid, false ) )
        {
        }

        FMoveOnlyPayload &operator=( FMoveOnlyPayload &&InOther ) noexcept
        {
            Value  = std::exchange( InOther.Value, -1 );
            bValid = std::exchange( InOther.bValid, false );
            return *this;
        }
    };

    struct FSequencedMessage
    {
        UInt32 ProducerIndex;
        UInt32 Sequence;
    };

    LUMEN_TEST( MPSCQueue_Initialization, FallibleInitializationAndEmptyState )
    {
        FIntQueue Queue;

        LUMEN_CHECK_FALSE( Queue.IsInitialized() );
        LUMEN_CHECK_EQ( Queue.GetCapacity(), 0ULL );
        LUMEN_CHECK_TRUE( Queue.IsEmpty() );
        LUMEN_CHECK_FALSE( Queue.Pop().HasValue() );

        const auto UninitializedPush = Queue.Push( 42 );
        LUMEN_CHECK_HAS_ERROR( UninitializedPush );
        LUMEN_CHECK_EQ( UninitializedPush.Error(), Thread::EMPSCQueueError::NotInitialized );

        const auto ZeroCapacity = Queue.Initialize( 0ULL );
        LUMEN_CHECK_HAS_ERROR( ZeroCapacity );
        LUMEN_CHECK_EQ( ZeroCapacity.Error(), Thread::EMPSCQueueError::InvalidCapacity );

        const auto OverflowingCapacity = Queue.Initialize( std::numeric_limits<USize>::max() );
        LUMEN_CHECK_HAS_ERROR( OverflowingCapacity );
        LUMEN_CHECK_EQ( OverflowingCapacity.Error(), Thread::EMPSCQueueError::InvalidCapacity );

        const auto Initialized = Queue.Initialize( 2ULL );
        LUMEN_CHECK_HAS_VALUE( Initialized );
        LUMEN_CHECK_TRUE( Queue.IsInitialized() );
        LUMEN_CHECK_EQ( Queue.GetCapacity(), 2ULL );

        const auto SecondInitialization = Queue.Initialize( 8ULL );
        LUMEN_CHECK_HAS_ERROR( SecondInitialization );
        LUMEN_CHECK_EQ( SecondInitialization.Error(), Thread::EMPSCQueueError::AlreadyInitialized );
        LUMEN_CHECK_EQ( Queue.GetCapacity(), 2ULL );

        LUMEN_CHECK_HAS_VALUE( Queue.Push( 42 ) );
        LUMEN_CHECK_EQ( *Queue.Pop(), 42 );
    }

    LUMEN_TEST( MPSCQueue_Capacity, AcceptsExactlyNAndRejectsWithoutAlteration )
    {
        constexpr Int32 Capacity = 3;
        FIntQueue Queue;
        LUMEN_CHECK_HAS_VALUE( Queue.Initialize( static_cast<USize>( Capacity ) ) );

        for ( Int32 Value = 0; Value < Capacity; ++Value )
        {
            LUMEN_CHECK_HAS_VALUE( Queue.Push( Value + 10 ) );
        }

        const Int32 RejectedValue = 99;
        const auto Rejected       = Queue.Push( RejectedValue );
        LUMEN_CHECK_HAS_ERROR( Rejected );
        LUMEN_CHECK_EQ( Rejected.Error(), Thread::EMPSCQueueError::QueueFull );
        LUMEN_CHECK_EQ( RejectedValue, 99 );

        for ( Int32 Value = 0; Value < Capacity; ++Value )
        {
            const TOptional<Int32> Item = Queue.Pop();
            LUMEN_CHECK_HAS_VALUE( Item );
            LUMEN_CHECK_EQ( *Item, Value + 10 );
        }

        LUMEN_CHECK_FALSE( Queue.Pop().HasValue() );
        LUMEN_CHECK_TRUE( Queue.IsEmpty() );
    }

    LUMEN_TEST( MPSCQueue_Capacity, SingleMessageCapacityIncludesConsumedStub )
    {
        FIntQueue Queue;
        LUMEN_CHECK_HAS_VALUE( Queue.Initialize( 1ULL ) );

        LUMEN_CHECK_HAS_VALUE( Queue.Push( 10 ) );
        const auto Rejected = Queue.Push( 20 );
        LUMEN_CHECK_HAS_ERROR( Rejected );
        LUMEN_CHECK_EQ( Rejected.Error(), Thread::EMPSCQueueError::QueueFull );
        LUMEN_CHECK_EQ( *Queue.Pop(), 10 );

        LUMEN_CHECK_HAS_VALUE( Queue.Push( 20 ) );
        LUMEN_CHECK_EQ( *Queue.Pop(), 20 );
        LUMEN_CHECK_FALSE( Queue.Pop().HasValue() );
    }

    LUMEN_TEST( MPSCQueue_Flow, PopRecyclesNodesForLaterFullBursts )
    {
        FIntQueue Queue;
        LUMEN_CHECK_HAS_VALUE( Queue.Initialize( 2ULL ) );

        for ( Int32 Round = 0; Round < 16; ++Round )
        {
            const Int32 Base = Round * 3;
            LUMEN_CHECK_HAS_VALUE( Queue.Push( Base ) );
            LUMEN_CHECK_HAS_VALUE( Queue.Push( Base + 1 ) );
            const auto Rejected = Queue.Push( -1 );
            LUMEN_CHECK_HAS_ERROR( Rejected );
            LUMEN_CHECK_EQ( Rejected.Error(), Thread::EMPSCQueueError::QueueFull );

            LUMEN_CHECK_EQ( *Queue.Pop(), Base );
            LUMEN_CHECK_HAS_VALUE( Queue.Push( Base + 2 ) );
            LUMEN_CHECK_EQ( *Queue.Pop(), Base + 1 );
            LUMEN_CHECK_EQ( *Queue.Pop(), Base + 2 );
            LUMEN_CHECK_FALSE( Queue.Pop().HasValue() );
        }
    }

    LUMEN_TEST( MPSCQueue_Push, FullMovePushPreservesRejectedPayload )
    {
        Thread::TMPSCQueue<FMoveOnlyPayload> Queue;
        LUMEN_CHECK_HAS_VALUE( Queue.Initialize( 1ULL ) );

        FMoveOnlyPayload First( 10 );
        FMoveOnlyPayload Rejected( 20 );

        LUMEN_CHECK_HAS_VALUE( Queue.Push( std::move( First ) ) );
        const auto RejectedResult = Queue.Push( std::move( Rejected ) );
        LUMEN_CHECK_HAS_ERROR( RejectedResult );
        LUMEN_CHECK_EQ( RejectedResult.Error(), Thread::EMPSCQueueError::QueueFull );
        LUMEN_CHECK_TRUE( Rejected.bValid );
        LUMEN_CHECK_EQ( Rejected.Value, 20 );

        const TOptional<FMoveOnlyPayload> FirstItem = Queue.Pop();
        LUMEN_CHECK_HAS_VALUE( FirstItem );
        LUMEN_CHECK_EQ( FirstItem->Value, 10 );

        LUMEN_CHECK_HAS_VALUE( Queue.Push( std::move( Rejected ) ) );
        LUMEN_CHECK_FALSE( Rejected.bValid );

        const TOptional<FMoveOnlyPayload> SecondItem = Queue.Pop();
        LUMEN_CHECK_HAS_VALUE( SecondItem );
        LUMEN_CHECK_EQ( SecondItem->Value, 20 );
    }

    LUMEN_TEST( MPSCQueue_Lifetime, PoppedAndRemainingPayloadsAreDestroyedExactlyOnce )
    {
        FPayloadLifetime Lifetime;

        {
            Thread::TMPSCQueue<FTrackedPayload> Queue;
            LUMEN_CHECK_HAS_VALUE( Queue.Initialize( 3ULL ) );

            for ( Int32 Value = 0; Value < 3; ++Value )
            {
                LUMEN_CHECK_HAS_VALUE( Queue.Push( FTrackedPayload( Lifetime, Value ) ) );
            }

            LUMEN_CHECK_EQ( Lifetime.LiveCount, 3 );

            {
                const TOptional<FTrackedPayload> Item = Queue.Pop();
                LUMEN_CHECK_HAS_VALUE( Item );
                LUMEN_CHECK_EQ( Item->Value, 0 );
                LUMEN_CHECK_EQ( Lifetime.LiveCount, 3 );
            }

            LUMEN_CHECK_EQ( Lifetime.LiveCount, 2 );
        }

        LUMEN_CHECK_EQ( Lifetime.LiveCount, 0 );
        LUMEN_CHECK_EQ( Lifetime.DestructionCount, Lifetime.ConstructionCount );
    }

    LUMEN_TEST( MPSCQueue_Lifetime, QueueDestructionDestroysEveryRemainingPayload )
    {
        FPayloadLifetime Lifetime;

        {
            Thread::TMPSCQueue<FTrackedPayload> Queue;
            LUMEN_CHECK_HAS_VALUE( Queue.Initialize( 4ULL ) );

            for ( Int32 Value = 0; Value < 4; ++Value )
            {
                LUMEN_CHECK_HAS_VALUE( Queue.Push( FTrackedPayload( Lifetime, Value ) ) );
            }

            LUMEN_CHECK_EQ( Lifetime.LiveCount, 4 );
        }

        LUMEN_CHECK_EQ( Lifetime.LiveCount, 0 );
        LUMEN_CHECK_EQ( Lifetime.DestructionCount, Lifetime.ConstructionCount );
    }

    LUMEN_TEST( MPSCQueue_Threading, BoundedProducersPreservePerProducerFifo )
    {
        constexpr UInt32 RunCount            = 32U;
        constexpr UInt32 ProducerCount       = 4U;
        constexpr UInt32 MessagesPerProducer = 250U;
        constexpr UInt32 TotalMessageCount   = ProducerCount * MessagesPerProducer;

        for ( UInt32 RunIndex = 0U; RunIndex < RunCount; ++RunIndex )
        {
            Thread::TMPSCQueue<FSequencedMessage> Queue;
            LUMEN_CHECK_HAS_VALUE( Queue.Initialize( 64ULL ) );

            auto Producer = [&Queue] ( const UInt32 InProducerIndex ) noexcept
            {
                for ( UInt32 Sequence = 0U; Sequence < MessagesPerProducer; ++Sequence )
                {
                    while ( not Queue.Push( FSequencedMessage{ InProducerIndex, Sequence } ).HasValue() )
                    {
                        FThread::Yield();
                    }
                }
            };

            TArray<FThread, ProducerCount> ProducerThreads = {
                FThread( Producer, 0U ),
                FThread( Producer, 1U ),
                FThread( Producer, 2U ),
                FThread( Producer, 3U ),
            };

            TArray<UInt32, ProducerCount> ExpectedSequences   = {};
            TArray<UInt32, ProducerCount> ReceivedPerProducer = {};
            UInt32 ReceivedMessageCount                       = 0U;
            Bool bFifoValid                                   = true;

            while ( ReceivedMessageCount < TotalMessageCount )
            {
                const TOptional<FSequencedMessage> Item = Queue.Pop();

                if ( not Item.HasValue() )
                {
                    FThread::Yield();
                    continue;
                }

                const FSequencedMessage Message = *Item;

                if ( Message.ProducerIndex >= ProducerCount )
                {
                    bFifoValid = false;
                }
                else
                {
                    if ( Message.Sequence != ExpectedSequences[Message.ProducerIndex] )
                    {
                        bFifoValid = false;
                    }

                    ExpectedSequences[Message.ProducerIndex] = Message.Sequence + 1U;
                    ++ReceivedPerProducer[Message.ProducerIndex];
                }

                ++ReceivedMessageCount;
            }

            for ( FThread &ProducerThread : ProducerThreads )
            {
                ProducerThread.Join();
            }

            LUMEN_CHECK_TRUE( bFifoValid );

            for ( UInt32 ProducerIndex = 0U; ProducerIndex < ProducerCount; ++ProducerIndex )
            {
                LUMEN_CHECK_EQ( ReceivedPerProducer[ProducerIndex], MessagesPerProducer );
                LUMEN_CHECK_EQ( ExpectedSequences[ProducerIndex], MessagesPerProducer );
            }

            LUMEN_CHECK_FALSE( Queue.Pop().HasValue() );
        }
    }

} // namespace

} // namespace Lumen
