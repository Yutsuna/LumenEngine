/**
 * @file ActorRegistry.cpp
 * @brief Implementation of the internal actor registry.
 */

#include <Actor/Actor.hpp>
#include <Actor/ActorRegistry.hpp>
#include <Actor/ActorTypes.hpp>

#include <Templates/Map.hpp>
#include <Templates/Pair.hpp>

#include <HAL/PlatformAtomic.hpp>
#include <HAL/SpinLock.hpp>

#include <Logging/Logger.hpp>

/**
 * Private
 */

namespace
{

static_assert( Lumen::TAtomic<Lumen::UInt64>::is_always_lock_free, "TAtomic<UInt64> must be lock-free" );

/**
 * @struct FSlot
 * @brief A slot in the actor registry.
 */
struct alignas( 64 ) FActorSlot
{
    Lumen::TAtomic<Lumen::UInt64> State   = { 0ULL };
    Lumen::TAtomic<Lumen::AActor *> Actor = { nullptr };
    Lumen::UInt32 NextFree                = 0U;
};

/**
 * @struct FChunk
 * @brief A chunk of actor slots in the actor registry.
 */
struct FActorChunk
{
    static constexpr Lumen::UInt32 ChunkBits = 10U;                   /** 10 bits per slot */
    static constexpr Lumen::UInt32 ChunkSize = 1U << ChunkBits;       /** 1024 slots per chunk */
    static constexpr Lumen::UInt32 ChunkMask = ChunkSize - 1U;        /** 1023 */
    static constexpr Lumen::UInt32 MaxChunks = 1U << 14U;             /** 16K chunks */
    static constexpr Lumen::UInt32 MaxSlots  = ChunkSize * MaxChunks; /** 16M live actors */

    FActorSlot Slots[ChunkSize];
};

/**
 * @struct FActorRegistryState
 * @brief The state of the actor registry.
 */
struct FActorRegistryState
{
    Lumen::HAL::FSpinLock Lock;
    Lumen::UInt32 NextSlot                                       = 1U; /** slot 0  is InvalidActorID */
    Lumen::UInt32 FreeHead                                       = 0U; /** 0 = empty */
    Lumen::TAtomic<FActorChunk *> Chunks[FActorChunk::MaxChunks] = {};

    /**
     * @brief Finds a slot in the registry by slot index.
     * @param InSlotIndex The slot index to find.
     * @return A pointer to the slot, or nullptr if the slot is not found.
     */
    [[nodiscard]] FActorSlot *FindSlot ( const Lumen::UInt32 InSlotIndex ) noexcept
    {
        const Lumen::UInt32 ChunkIndex = InSlotIndex >> FActorChunk::ChunkBits;

        if ( InSlotIndex == 0U or ChunkIndex >= FActorChunk::MaxChunks ) [[unlikely]]
        {
            return nullptr;
        }

        FActorChunk *const Chunk = this->Chunks[ChunkIndex].load( std::memory_order_acquire );
        return Chunk != nullptr ? &Chunk->Slots[InSlotIndex & FActorChunk::ChunkMask] : nullptr;
    }

    /**
     * @brief Acquires a slot in the registry.
     * @return The slot index, or 0 if no slot could be acquired.
     */
    [[nodiscard]] Lumen::UInt32 AcquireSlot () noexcept
    {
        Lumen::HAL::FSpinGuard Guard( Lock );

        if ( const Lumen::UInt32 Recycled = FreeHead; Recycled != 0U )
        {
            FreeHead = FindSlot( Recycled )->NextFree;
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

[[nodiscard]] Lumen::UInt32 GenerationOf ( const FActorSlot &InSlot, const std::memory_order InOrder ) noexcept
{
    return static_cast<Lumen::UInt32>( InSlot.State.load( InOrder ) );
}

constinit FActorRegistryState GRegistry;

constexpr Lumen::UInt64 GenerationMask = 0xFFFFFFFFULL;
constexpr Lumen::UInt64 ReaderUnit     = 1ULL << 32U;

} // namespace

/**
 * Public
 */

Lumen::ActorID Lumen::Internal::FActorRegistry::AllocateId () noexcept
{
    const UInt32 SlotIndex = GRegistry.AcquireSlot();

    if ( SlotIndex == 0U ) [[unlikely]]
    {
        LUMEN_LOG_ERROR( LogActor, "FActorRegistry::AllocateId: failed to acquire slot, registry exhausted." );
        return InvalidActorID;
    }

    /** INFO: Bumped at release time so the ID carries the generation of the next occupant. */
    return MakeActorId( SlotIndex, GenerationOf( *GRegistry.FindSlot( SlotIndex ), std::memory_order_relaxed ) );
}

void Lumen::Internal::FActorRegistry::Register ( AActor *InActor ) noexcept
{
    if ( InActor == nullptr ) [[unlikely]]
    {
        LUMEN_LOG_WARNING( LogActor, "FActorRegistry::Register: Actor is null." );
        return;
    }

    const ActorID Id       = InActor->GetId();
    FActorSlot *const Slot = GRegistry.FindSlot( ActorIdSlotIndex( Id ) );

    if ( Slot == nullptr ) [[unlikely]]
    {
        LUMEN_LOG_ERROR( LogActor,
                         "FActorRegistry::Register: ActorID '{}' maps to an unallocated chunk (was AllocateId() called?).", Id );
        return;
    }

    if ( GenerationOf( *Slot, std::memory_order_relaxed ) != ActorIdGeneration( Id ) ) [[unlikely]]
    {
        LUMEN_LOG_ERROR( LogActor, "FActorRegistry::Register: ActorID '{}' generation mismatch.", Id );
        return;
    }

    AActor *Expected = nullptr;

    if ( not Slot->Actor.compare_exchange_strong( Expected, InActor, std::memory_order_release, std::memory_order_relaxed ) )
        [[unlikely]]
    {
        LUMEN_LOG_ERROR( LogActor, "FActorRegistry::Register: ActorID '{}' already registered.", Id );
    }
}

void Lumen::Internal::FActorRegistry::Unregister ( AActor *InActor ) noexcept
{
    if ( InActor == nullptr ) [[unlikely]]
    {
        LUMEN_LOG_WARNING( LogActor, "FActorRegistry::Unregister: Actor is null." );
        return;
    }

    const UInt32 SlotIndex = ActorIdSlotIndex( InActor->GetId() );
    FActorSlot *const Slot = GRegistry.FindSlot( SlotIndex );

    /** INFO: Only the registered owner retires a slot; a rejected duplicate must not. */
    if ( Slot == nullptr or Slot->Actor.load( std::memory_order_relaxed ) != InActor )
    {
        return;
    }

    UInt64 OldState = Slot->State.load( std::memory_order_relaxed );
    UInt64 NewState = 0ULL;

    do
    {
        NewState = ( OldState & ~GenerationMask ) | ( ( OldState + 1ULL ) & GenerationMask );
    } while ( not Slot->State.compare_exchange_weak( OldState, NewState, std::memory_order_acq_rel, std::memory_order_relaxed ) );

    while ( ( Slot->State.load( std::memory_order_acquire ) >> 32U ) != 0ULL )
    {
        HAL::CpuRelax();
    }

    Slot->Actor.store( nullptr, std::memory_order_release );

    HAL::FSpinGuard Guard( GRegistry.Lock );
    Slot->NextFree     = GRegistry.FreeHead;
    GRegistry.FreeHead = SlotIndex;
}

void Lumen::Internal::FActorRegistry::EnqueueMessage ( ActorID InId, const FMessage &InMessage ) noexcept
{
    FActorSlot *const Slot = GRegistry.FindSlot( ActorIdSlotIndex( InId ) );

    if ( Slot == nullptr )
    {
        return;
    }

    const UInt32 Generation = ActorIdGeneration( InId );

    /** INFO: Keep stale senders from pinning a retiring slot. */
    if ( GenerationOf( *Slot, std::memory_order_relaxed ) != Generation ) [[unlikely]]
    {
        LUMEN_LOG_WARNING( LogActor, "FActorRegistry::EnqueueMessage: ActorID '{}' generation mismatch.", InId );
        return;
    }

    /** INFO: Pin + generation check in one RMW operation. */
    const UInt64 Previous = Slot->State.fetch_add( ReaderUnit, std::memory_order_acquire );
    if ( static_cast<UInt32>( Previous ) == Generation ) [[likely]]
    {
        if ( AActor *const Target = Slot->Actor.load( std::memory_order_acquire ) ) [[likely]]
        {
            Target->EnqueueMessage( InMessage );
        }
    }

    /** INFO: Unpin after message dispatch. */
    Slot->State.fetch_sub( ReaderUnit, std::memory_order_release );
}
