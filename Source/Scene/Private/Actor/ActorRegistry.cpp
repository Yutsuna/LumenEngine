/**
 * @file ActorRegistry.cpp
 * @brief Implementation of the internal actor registry.
 */

#include <Actor/Actor.hpp>
#include <Actor/ActorRegistry.hpp>
#include <Actor/ActorTypes.hpp>

#include <Templates/Array.hpp>
#include <Templates/Pair.hpp>

#include <HAL/LockGuard.hpp>
#include <HAL/PlatformAtomic.hpp>
#include <HAL/PlatformMutex.hpp>

#include <Logging/Logger.hpp>

namespace
{

static_assert( Lumen::TAtomic<Lumen::UInt64>::is_always_lock_free, "TAtomic<UInt64> must be lock-free" );

struct alignas( 64 ) FActorSlot
{
    Lumen::TAtomic<Lumen::UInt64> State   = { 0ULL };
    Lumen::TAtomic<Lumen::AActor *> Actor = { nullptr };
    Lumen::TAtomic<Lumen::Bool> Routable  = { false };
    Lumen::UInt32 NextFree                = 0U;
};

struct FActorChunk
{
    static constexpr Lumen::UInt32 ChunkBits = 10U;
    static constexpr Lumen::UInt32 ChunkSize = 1U << ChunkBits;
    static constexpr Lumen::UInt32 ChunkMask = ChunkSize - 1U;
    static constexpr Lumen::UInt32 MaxChunks = 1U << 14U;
    static constexpr Lumen::UInt32 MaxSlots  = ChunkSize * MaxChunks;

    FActorSlot Slots[ChunkSize];
};

struct FActorRegistryState
{
    Lumen::HAL::FPlatformSpinLockMutex Lock;
    Lumen::UInt32 NextSlot                                       = 1U;
    Lumen::UInt32 FreeHead                                       = 0U;
    Lumen::TAtomic<FActorChunk *> Chunks[FActorChunk::MaxChunks] = {};

    ~FActorRegistryState () noexcept
    {
        for ( Lumen::UInt32 ChunkIndex = 0U; ChunkIndex < FActorChunk::MaxChunks; ++ChunkIndex )
        {
            if ( FActorChunk *const Chunk = Chunks[ChunkIndex].load( std::memory_order_acquire ) )
            {
                delete Chunk;
                Chunks[ChunkIndex].store( nullptr, std::memory_order_relaxed );
            }
        }
    }

    [[nodiscard]] FActorSlot *FindSlot ( const Lumen::UInt32 InSlotIndex ) noexcept
    {
        const Lumen::UInt32 ChunkIndex = InSlotIndex >> FActorChunk::ChunkBits;

        if ( InSlotIndex == 0U or ChunkIndex >= FActorChunk::MaxChunks ) [[unlikely]]
        {
            return nullptr;
        }

        FActorChunk *const Chunk = Chunks[ChunkIndex].load( std::memory_order_acquire );
        return Chunk != nullptr ? &Chunk->Slots[InSlotIndex & FActorChunk::ChunkMask] : nullptr;
    }

    [[nodiscard]] Lumen::UInt32 AcquireSlot () noexcept
    {
        Lumen::HAL::TLockGuard<Lumen::HAL::FPlatformSpinLockMutex> Guard( Lock );

        if ( const Lumen::UInt32 Recycled = FreeHead; Recycled != 0U )
        {
            FActorSlot *const RecycledSlot = FindSlot( Recycled );
            if ( RecycledSlot == nullptr ) [[unlikely]]
            {
                return 0U;
            }
            FreeHead = RecycledSlot->NextFree;
            return Recycled;
        }

        if ( NextSlot >= FActorChunk::MaxSlots )
        {
            return 0U;
        }

        const Lumen::UInt32 FreshSlot               = NextSlot;
        Lumen::TAtomic<FActorChunk *> &ChunkPointer = Chunks[FreshSlot >> FActorChunk::ChunkBits];

        if ( ChunkPointer.load( std::memory_order_relaxed ) == nullptr )
        {
            FActorChunk *const NewChunk = new ( std::nothrow ) FActorChunk();
            if ( NewChunk == nullptr ) [[unlikely]]
            {
                return 0U;
            }

            ChunkPointer.store( NewChunk, std::memory_order_release );
        }

        ++NextSlot;
        return FreshSlot;
    }
};

struct FDeadLetterDiagnostics
{
    Lumen::HAL::FPlatformSpinLockMutex Lock;
    Lumen::TArray<Lumen::FDeadLetterRecord, Lumen::Internal::FActorRegistry::DeadLetterCapacity> Records = {};
    Lumen::UInt64 TotalRecorded                                                                          = 0ULL;
};

[[nodiscard]] Lumen::UInt32 GenerationOf ( const FActorSlot &InSlot, const std::memory_order InOrder ) noexcept
{
    return static_cast<Lumen::UInt32>( InSlot.State.load( InOrder ) );
}

constinit FActorRegistryState GRegistry;
constinit FDeadLetterDiagnostics GDeadLetterDiagnostics;
constinit Lumen::TAtomic<Lumen::UInt64> GDeadLetters{ 0ULL };
constinit Lumen::TArray<Lumen::TAtomic<Lumen::UInt64>, static_cast<Lumen::USize>( Lumen::EActorSendError::Count )>
    GDeadLettersByReason = {};

constexpr Lumen::UInt64 GenerationMask = 0xFFFFFFFFULL;
constexpr Lumen::UInt64 ReaderUnit     = 1ULL << 32U;

void RetireSlot ( const Lumen::ActorID InId, FActorSlot &InSlot, const Lumen::Bool bRequireUnbound ) noexcept
{
    if ( GenerationOf( InSlot, std::memory_order_relaxed ) != Lumen::ActorIdGeneration( InId ) )
    {
        return;
    }

    if ( bRequireUnbound and InSlot.Actor.load( std::memory_order_relaxed ) != nullptr )
    {
        return;
    }

    InSlot.Routable.store( false, std::memory_order_release );

    Lumen::UInt64 OldState = InSlot.State.load( std::memory_order_relaxed );
    Lumen::UInt64 NewState = 0ULL;

    do
    {
        NewState = ( OldState & ~GenerationMask ) | ( ( OldState + 1ULL ) & GenerationMask );
    } while (
        not InSlot.State.compare_exchange_weak( OldState, NewState, std::memory_order_acq_rel, std::memory_order_relaxed ) );

    while ( ( InSlot.State.load( std::memory_order_acquire ) >> 32U ) != 0ULL )
    {
        Lumen::HAL::CpuRelax();
    }

    InSlot.Actor.store( nullptr, std::memory_order_release );

    Lumen::HAL::TLockGuard<Lumen::HAL::FPlatformSpinLockMutex> Guard( GRegistry.Lock );
    InSlot.NextFree    = GRegistry.FreeHead;
    GRegistry.FreeHead = Lumen::ActorIdSlotIndex( InId );
}

} // namespace

Lumen::ActorID Lumen::Internal::FActorRegistry::AllocateId () noexcept
{
    const UInt32 SlotIndex = GRegistry.AcquireSlot();

    if ( SlotIndex == 0U ) [[unlikely]]
    {
        LUMEN_LOG_ERROR( LogActor, "FActorRegistry::AllocateId: failed to acquire slot, registry exhausted." );
        return InvalidActorID;
    }

    return MakeActorId( SlotIndex, GenerationOf( *GRegistry.FindSlot( SlotIndex ), std::memory_order_relaxed ) );
}

Lumen::UInt64 Lumen::Internal::FActorRegistry::GetDeadLetterCount () noexcept
{
    return GDeadLetters.load( std::memory_order_relaxed );
}

Lumen::UInt64 Lumen::Internal::FActorRegistry::GetDeadLetterCount ( const EActorSendError InReason ) noexcept
{
    const USize ReasonIndex = static_cast<USize>( InReason );
    if ( ReasonIndex >= GDeadLettersByReason.size() )
    {
        return 0ULL;
    }
    return GDeadLettersByReason[ReasonIndex].load( std::memory_order_relaxed );
}

Lumen::USize Lumen::Internal::FActorRegistry::GetDeadLetterRecordCount () noexcept
{
    HAL::TLockGuard<HAL::FPlatformSpinLockMutex> Guard( GDeadLetterDiagnostics.Lock );
    return static_cast<USize>( GDeadLetterDiagnostics.TotalRecorded < DeadLetterCapacity ? GDeadLetterDiagnostics.TotalRecorded
                                                                                         : DeadLetterCapacity );
}

Lumen::UInt64 Lumen::Internal::FActorRegistry::GetDroppedDeadLetterRecordCount () noexcept
{
    HAL::TLockGuard<HAL::FPlatformSpinLockMutex> Guard( GDeadLetterDiagnostics.Lock );
    return GDeadLetterDiagnostics.TotalRecorded > DeadLetterCapacity ? GDeadLetterDiagnostics.TotalRecorded - DeadLetterCapacity
                                                                     : 0ULL;
}

Lumen::USize Lumen::Internal::FActorRegistry::ReadDeadLetters ( const TSpan<FDeadLetterRecord> OutRecords ) noexcept
{
    HAL::TLockGuard<HAL::FPlatformSpinLockMutex> Guard( GDeadLetterDiagnostics.Lock );

    const UInt64 Retained =
        GDeadLetterDiagnostics.TotalRecorded < DeadLetterCapacity ? GDeadLetterDiagnostics.TotalRecorded : DeadLetterCapacity;
    const UInt64 FirstAvailable = GDeadLetterDiagnostics.TotalRecorded - Retained;
    const UInt64 FirstRequested = GDeadLetterDiagnostics.TotalRecorded > OutRecords.size()
                                      ? GDeadLetterDiagnostics.TotalRecorded - OutRecords.size()
                                      : 0ULL;
    const UInt64 FirstSequence  = FirstRequested > FirstAvailable ? FirstRequested : FirstAvailable;
    const USize CopyCount       = static_cast<USize>( GDeadLetterDiagnostics.TotalRecorded - FirstSequence );

    for ( USize Index = 0ULL; Index < CopyCount; ++Index )
    {
        OutRecords[Index] = GDeadLetterDiagnostics.Records[( FirstSequence + Index ) % DeadLetterCapacity];
    }
    return CopyCount;
}

Lumen::Bool Lumen::Internal::FActorRegistry::Register ( AActor *const InActor ) noexcept
{
    if ( InActor == nullptr ) [[unlikely]]
    {
        return false;
    }

    const ActorID Id       = InActor->GetId();
    FActorSlot *const Slot = GRegistry.FindSlot( ActorIdSlotIndex( Id ) );

    if ( Slot == nullptr or GenerationOf( *Slot, std::memory_order_relaxed ) != ActorIdGeneration( Id ) ) [[unlikely]]
    {
        return false;
    }

    AActor *Expected = nullptr;
    if ( not Slot->Actor.compare_exchange_strong( Expected, InActor, std::memory_order_release, std::memory_order_relaxed ) )
        [[unlikely]]
    {
        return false;
    }

    Slot->Routable.store( true, std::memory_order_release );
    return true;
}

void Lumen::Internal::FActorRegistry::OpenRouting ( AActor *const InActor ) noexcept
{
    if ( InActor == nullptr )
    {
        return;
    }

    FActorSlot *const Slot = GRegistry.FindSlot( ActorIdSlotIndex( InActor->GetId() ) );
    if ( Slot != nullptr and Slot->Actor.load( std::memory_order_acquire ) == InActor )
    {
        Slot->Routable.store( true, std::memory_order_release );
    }
}

void Lumen::Internal::FActorRegistry::CloseRouting ( AActor *const InActor ) noexcept
{
    if ( InActor == nullptr )
    {
        return;
    }

    FActorSlot *const Slot = GRegistry.FindSlot( ActorIdSlotIndex( InActor->GetId() ) );
    if ( Slot != nullptr and Slot->Actor.load( std::memory_order_acquire ) == InActor )
    {
        Slot->Routable.store( false, std::memory_order_release );
    }
}

void Lumen::Internal::FActorRegistry::Unregister ( AActor *const InActor ) noexcept
{
    if ( InActor == nullptr ) [[unlikely]]
    {
        return;
    }

    const ActorID Id       = InActor->GetId();
    FActorSlot *const Slot = GRegistry.FindSlot( ActorIdSlotIndex( Id ) );

    if ( Slot == nullptr or Slot->Actor.load( std::memory_order_relaxed ) != InActor )
    {
        return;
    }

    RetireSlot( Id, *Slot, false );
}

void Lumen::Internal::FActorRegistry::EnqueueMessage ( const ActorID InId, const FMessage &InMessage ) noexcept
{
    static_cast<void>( TryEnqueueMessage( InId, InMessage ) );
}

Lumen::TExpected<void, Lumen::EActorSendError> Lumen::Internal::FActorRegistry::TryEnqueueMessage (
    const ActorID InId, const FMessage &InMessage, const UInt64 InFrameIndex, const UInt64 InSenderSequence ) noexcept
{
    if ( InId == InvalidActorID )
    {
        return MakeUnexpected( EActorSendError::InvalidActor );
    }

    EActorSendError Failure = EActorSendError::UnknownActor;
    FActorSlot *const Slot  = GRegistry.FindSlot( ActorIdSlotIndex( InId ) );

    if ( Slot != nullptr )
    {
        const UInt32 Generation = ActorIdGeneration( InId );
        if ( GenerationOf( *Slot, std::memory_order_relaxed ) != Generation )
        {
            Failure = EActorSendError::StaleActor;
        }
        else
        {
            const UInt64 Previous = Slot->State.fetch_add( ReaderUnit, std::memory_order_acquire );
            if ( static_cast<UInt32>( Previous ) == Generation )
            {
                AActor *const Target = Slot->Actor.load( std::memory_order_acquire );
                if ( Target == nullptr )
                {
                    Failure = EActorSendError::UnknownActor;
                }
                else if ( not Slot->Routable.load( std::memory_order_acquire ) )
                {
                    Failure = EActorSendError::ActorRetiring;
                }
                else if ( Target->EnqueueMessage( InMessage ) )
                {
                    Slot->State.fetch_sub( ReaderUnit, std::memory_order_release );
                    return {};
                }
                else if ( Target->GetLifecycle() != EActorLifecycle::Active or
                          not Slot->Routable.load( std::memory_order_acquire ) )
                {
                    Failure = EActorSendError::ActorRetiring;
                }
                else
                {
                    Failure = EActorSendError::MailboxFull;
                }
            }
            else
            {
                Failure = EActorSendError::StaleActor;
            }

            Slot->State.fetch_sub( ReaderUnit, std::memory_order_release );
        }
    }

    RecordDeadLetter( Failure, InId, InMessage, InFrameIndex, InSenderSequence );
    return MakeUnexpected( Failure );
}

void Lumen::Internal::FActorRegistry::RecordDeadLetter ( const EActorSendError InReason,
                                                         const ActorID InTarget,
                                                         const FMessage &InMessage,
                                                         const UInt64 InFrameIndex,
                                                         const UInt64 InSenderSequence ) noexcept
{
    const USize ReasonIndex = static_cast<USize>( InReason );
    if ( InTarget == InvalidActorID or ReasonIndex >= GDeadLettersByReason.size() )
    {
        return;
    }

    GDeadLetters.fetch_add( 1ULL, std::memory_order_relaxed );
    GDeadLettersByReason[ReasonIndex].fetch_add( 1ULL, std::memory_order_relaxed );

    HAL::TLockGuard<HAL::FPlatformSpinLockMutex> Guard( GDeadLetterDiagnostics.Lock );
    const USize RecordIndex                     = static_cast<USize>( GDeadLetterDiagnostics.TotalRecorded % DeadLetterCapacity );
    GDeadLetterDiagnostics.Records[RecordIndex] = FDeadLetterRecord{ .Target         = InTarget,
                                                                     .Sender         = InMessage.Sender,
                                                                     .FrameIndex     = InFrameIndex,
                                                                     .SenderSequence = InSenderSequence,
                                                                     .Type           = InMessage.Type,
                                                                     .Reason         = InReason };
    ++GDeadLetterDiagnostics.TotalRecorded;
}

void Lumen::Internal::FActorRegistry::ReleaseId ( const ActorID InId ) noexcept
{
    if ( InId == InvalidActorID )
    {
        return;
    }

    FActorSlot *const Slot = GRegistry.FindSlot( ActorIdSlotIndex( InId ) );
    if ( Slot != nullptr )
    {
        RetireSlot( InId, *Slot, true );
    }
}
