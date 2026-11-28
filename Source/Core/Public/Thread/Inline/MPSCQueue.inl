/**
 * @file MPSCQueue.inl
 * @brief Inline implementation for TMPSCQueue.
 */

#pragma once

#include <Thread/MPSCQueue.hpp>

#include <HAL/PlatformThread.hpp>

#include <Templates/Unexpected.hpp>

#include <limits>
#include <new>
#include <utility>

template <typename Type>
Lumen::TExpected<void, Lumen::Thread::EMPSCQueueError>
Lumen::Thread::TMPSCQueue<Type>::Initialize ( const USize InCapacity ) noexcept
{
    if ( IsInitialized() ) [[unlikely]]
    {
        return TUnexpected<EMPSCQueueError>( EMPSCQueueError::AlreadyInitialized );
    }

    constexpr USize MaxUSize = std::numeric_limits<USize>::max();

    if ( InCapacity == 0ULL or InCapacity == MaxUSize ) [[unlikely]]
    {
        return TUnexpected<EMPSCQueueError>( EMPSCQueueError::InvalidCapacity );
    }

    USize FreeCapacity = 2ULL;

    while ( FreeCapacity < InCapacity )
    {
        if ( FreeCapacity > MaxUSize / 2ULL ) [[unlikely]]
        {
            return TUnexpected<EMPSCQueueError>( EMPSCQueueError::InvalidCapacity );
        }

        FreeCapacity *= 2ULL;
    }

    const USize NodeCount = InCapacity + 1ULL;

    if ( NodeCount > MaxUSize / sizeof( FNode ) or FreeCapacity > MaxUSize / sizeof( FFreeCell ) ) [[unlikely]]
    {
        return TUnexpected<EMPSCQueueError>( EMPSCQueueError::InvalidCapacity );
    }

    TUniquePtr<FNode[]> NewNodes( new ( std::nothrow ) FNode[NodeCount] );

    if ( not NewNodes.IsValid() ) [[unlikely]]
    {
        return TUnexpected<EMPSCQueueError>( EMPSCQueueError::OutOfMemory );
    }

    TUniquePtr<FFreeCell[]> NewFreeStorage( new ( std::nothrow ) FFreeCell[FreeCapacity] );

    if ( not NewFreeStorage.IsValid() ) [[unlikely]]
    {
        return TUnexpected<EMPSCQueueError>( EMPSCQueueError::OutOfMemory );
    }

    FFreeCell *const Cells = NewFreeStorage.Get();

    for ( USize Index = 0ULL; Index < FreeCapacity; ++Index )
    {
        Cells[Index].Sequence.store( Index, std::memory_order_relaxed );
    }

    FNode *const NodeStorage = NewNodes.Get();

    for ( USize Index = 0ULL; Index < InCapacity; ++Index )
    {
        Cells[Index].FreeNode = &NodeStorage[Index + 1ULL];
        Cells[Index].Sequence.store( Index + 1ULL, std::memory_order_relaxed );
    }

    Nodes       = std::move( NewNodes );
    FreeStorage = std::move( NewFreeStorage );

    Capacity            = InCapacity;
    FreeCapacityMask    = FreeCapacity - 1ULL;
    FreeEnqueuePosition = InCapacity;
    FreeDequeuePosition.store( 0ULL, std::memory_order_relaxed );
    Head = Nodes.Get();
    Tail.store( Head, std::memory_order_relaxed );
    return {};
}

template <typename Type>
Lumen::TExpected<void, Lumen::Thread::EMPSCQueueError> Lumen::Thread::TMPSCQueue<Type>::Push ( const Type &InData ) noexcept
    requires std::is_nothrow_copy_constructible_v<Type>
{
    return PushImpl( InData );
}

template <typename Type>
Lumen::TExpected<void, Lumen::Thread::EMPSCQueueError> Lumen::Thread::TMPSCQueue<Type>::Push ( Type &&InData ) noexcept
{
    return PushImpl( std::move( InData ) );
}

template <typename Type>
template <typename ArgumentType>
Lumen::TExpected<void, Lumen::Thread::EMPSCQueueError>
Lumen::Thread::TMPSCQueue<Type>::PushImpl ( ArgumentType &&InData ) noexcept
{
    static_assert( std::is_nothrow_constructible_v<Type, ArgumentType>,
                   "TMPSCQueue::Push requires a nothrow payload construction." );

    if ( not IsInitialized() ) [[unlikely]]
    {
        return TUnexpected<EMPSCQueueError>( EMPSCQueueError::NotInitialized );
    }

    FNode *const NewNode = AcquireNode();

    if ( NewNode == nullptr ) [[unlikely]]
    {
        return TUnexpected<EMPSCQueueError>( EMPSCQueueError::QueueFull );
    }

    NewNode->Next.store( nullptr, std::memory_order_relaxed );
    NewNode->Data.Emplace( std::forward<ArgumentType>( InData ) );

    /** The exchange gives concurrent insertions their list order. */
    FNode *const PreviousTail = Tail.exchange( NewNode, std::memory_order_acq_rel );

    /** Publishing Next releases the initialized payload to the consumer. */
    PreviousTail->Next.store( NewNode, std::memory_order_release );
    return {};
}

template <typename Type> Lumen::TOptional<Type> Lumen::Thread::TMPSCQueue<Type>::Pop () noexcept
{
    if ( Head == nullptr ) [[unlikely]]
    {
        return {};
    }

    FNode *const NextNode = Head->Next.load( std::memory_order_acquire );

    if ( NextNode == nullptr )
    {
        return {};
    }

    LUMEN_ASSERT( NextNode->Data.HasValue() );

    TOptional<Type> Result( std::move( *NextNode->Data ) );
    NextNode->Data.Reset();

    FNode *const OldHead = std::exchange( Head, NextNode );
    RecycleHead( OldHead );
    return Result;
}

template <typename Type> Lumen::Bool Lumen::Thread::TMPSCQueue<Type>::IsInitialized () const noexcept
{
    return Nodes.IsValid();
}

template <typename Type> Lumen::USize Lumen::Thread::TMPSCQueue<Type>::GetCapacity () const noexcept
{
    return Capacity;
}

template <typename Type> Lumen::Bool Lumen::Thread::TMPSCQueue<Type>::IsEmpty () const noexcept
{
    return Head == nullptr or Head->Next.load( std::memory_order_acquire ) == nullptr;
}

template <typename Type> Lumen::Thread::TMPSCQueue<Type>::FNode *Lumen::Thread::TMPSCQueue<Type>::AcquireNode () noexcept
{
    USize Position  = FreeDequeuePosition.load( std::memory_order_relaxed );
    FFreeCell *Cell = nullptr;

    for ( ;; )
    {
        Cell                  = &FreeStorage[Position & FreeCapacityMask];
        const USize Sequence  = Cell->Sequence.load( std::memory_order_acquire );
        const IPtr Difference = static_cast<IPtr>( Sequence ) - static_cast<IPtr>( Position + 1ULL );

        if ( Difference == 0 )
        {
            if ( FreeDequeuePosition.compare_exchange_weak( Position, Position + 1ULL, std::memory_order_relaxed,
                                                            std::memory_order_relaxed ) )
            {
                break;
            }
        }
        else if ( Difference < 0 )
        {
            return nullptr;
        }
        else
        {
            Position = FreeDequeuePosition.load( std::memory_order_relaxed );
        }
    }

    FNode *const Result = Cell->FreeNode;
    Cell->FreeNode      = nullptr;
    Cell->Sequence.store( Position + FreeCapacityMask + 1ULL, std::memory_order_release );
    return Result;
}

template <typename Type> void Lumen::Thread::TMPSCQueue<Type>::RecycleHead ( FNode *const InOldHead ) noexcept
{
    LUMEN_ASSERT( InOldHead != nullptr and not InOldHead->Data.HasValue() );

    FFreeCell *const Cell = &FreeStorage[FreeEnqueuePosition & FreeCapacityMask];

    /**
     * A producer publishes a cell's next sequence only after claiming the slot by advancing FreeDequeuePosition. It can be
     * preempted between those two steps, so the cell can still carry the previous cycle's sequence when the single free-list
     * producer reaches it. Wait for that in-flight producer rather than dropping the node, which would permanently shrink the
     * free ring. Every recycled node was acquired first, so the ring can never overflow.
     */
    while ( Cell->Sequence.load( std::memory_order_acquire ) != FreeEnqueuePosition )
    {
        HAL::FPlatformThread::Yield();
    }

    InOldHead->Next.store( nullptr, std::memory_order_relaxed );
    Cell->FreeNode = InOldHead;
    Cell->Sequence.store( FreeEnqueuePosition + 1ULL, std::memory_order_release );
    ++FreeEnqueuePosition;
}
