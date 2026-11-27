/**
 * @file ActorRegistry.hpp
 * @brief Internal registry for mapping ActorIDs to AActor pointers.
 */

#pragma once

#include <Actor/ActorTypes.hpp>
#include <SceneExport.hpp>

#include <Memory/Span.hpp>
#include <Templates/Expected.hpp>

namespace Lumen
{

/**
 * @struct FDeadLetterRecord
 * @brief Bounded diagnostic metadata for a message that was not delivered. */
struct SCENE_EXPORT FDeadLetterRecord
{
    ActorID Target         = 0ULL;
    ActorID Sender         = 0ULL;
    UInt64 FrameIndex      = 0ULL;
    UInt64 SenderSequence  = 0ULL;
    UInt32 Type            = 0U;
    EActorSendError Reason = EActorSendError::InvalidActor;
};

namespace Internal
{
    /**
     * @class FActorRegistry
     * @brief Thread-safe registry for actor lookups and logical route closure.
     * @note Sends are lock-free; spawn/despawn and diagnostic snapshots take a short lock.
     */
    class SCENE_EXPORT FActorRegistry
    {
    public:

        static constexpr USize DeadLetterCapacity = 256ULL;

        /** @brief Allocates a fresh ActorID or returns RegistryExhausted. Safe from any thread. */
        [[nodiscard]] static TExpected<ActorID, EActorSpawnError> AllocateId () noexcept;

        /** @brief Total number of dead letters, preserving the historical counter contract. */
        [[nodiscard]] static UInt64 GetDeadLetterCount () noexcept;

        /** @brief Total number of dead letters for one structured reason. */
        [[nodiscard]] static UInt64 GetDeadLetterCount ( EActorSendError InReason ) noexcept;

        /** @brief Number of diagnostic records currently retained. */
        [[nodiscard]] static USize GetDeadLetterRecordCount () noexcept;

        /** @brief Number of diagnostic records overwritten after capacity was reached. */
        [[nodiscard]] static UInt64 GetDroppedDeadLetterRecordCount () noexcept;

        /** @brief Copies the newest retained records in chronological order. */
        static USize ReadDeadLetters ( TSpan<FDeadLetterRecord> OutRecords ) noexcept;

        /** @brief Registers the actor in its slot. Serial phase only. */
        [[nodiscard]] static Bool Register ( AActor *InActor ) noexcept;

        /** @brief Opens routing after deferred activation. Serial phase only. */
        static void OpenRouting ( AActor *InActor ) noexcept;

        /** @brief Closes routing before C++ destruction. Safe during concurrent sends. */
        static void CloseRouting ( AActor *InActor ) noexcept;

        /** @brief Nulls the slot and bumps its generation. Serial phase only. */
        static void Unregister ( AActor *InActor ) noexcept;

        /** @brief Legacy fire-and-forget delivery adapter. */
        static void EnqueueMessage ( ActorID InId, const FMessage &InMessage ) noexcept;

        /** @brief Lock-free lookup + fallible delivery with structured dead-lettering. */
        [[nodiscard]] static TExpected<void, EActorSendError> TryEnqueueMessage ( ActorID InId,
                                                                                  const FMessage &InMessage,
                                                                                  UInt64 InFrameIndex     = 0ULL,
                                                                                  UInt64 InSenderSequence = 0ULL ) noexcept;

        /** @brief Records a delivery failure without changing the legacy invalid-ID exception. */
        static void RecordDeadLetter ( EActorSendError InReason,
                                       ActorID InTarget,
                                       const FMessage &InMessage,
                                       UInt64 InFrameIndex     = 0ULL,
                                       UInt64 InSenderSequence = 0ULL ) noexcept;

        /** @brief Retires an unbound ID whose deferred spawn was cancelled. */
        static void ReleaseId ( ActorID InId ) noexcept;
    };

} // namespace Internal

} // namespace Lumen
