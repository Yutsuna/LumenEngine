/// @file ActorBenchmarkHelpers.hpp
/// @brief Shared fixtures used by the actor benchmark suites.
///
/// The helpers keep the benchmark bodies focused on the measured path: a fixed message, actors without user logic, and
/// a non-timed mailbox drain used to reset a producer/consumer state between measured operations.

#pragma once

#include <Actor/Actor.hpp>

#include <Benchmarks/BenchmarkOptimizer.hpp>

namespace Lumen
{

///
/// @struct FActorBenchmarkPayload
/// @brief Small trait-identified payload used to measure message decoding.
///
struct FActorBenchmarkPayload
{
    UInt64 Sequence{ 0ULL };
};

template <> struct TMessageTraits<FActorBenchmarkPayload>
{
    static constexpr Bool bEnabled  = true;
    static constexpr UInt32 Type    = 1U;
    static constexpr UInt16 Version = 1U;
};

///
/// @class FNoOpBenchmarkActor
/// @brief Minimal actor isolating mailbox and dispatch overhead from user code.
///
class FNoOpBenchmarkActor final : public AActor
{
    LUMEN_ACTOR_BODY( FNoOpBenchmarkActor )

public:

    using AActor::AActor;

    void Receive ( const FMessage & ) noexcept override
    {
        ///
    }
};

///
/// @class FPayloadBenchmarkActor
/// @brief Actor decoding the benchmark payload on every message.
///
class FPayloadBenchmarkActor final : public AActor
{
    LUMEN_ACTOR_BODY( FPayloadBenchmarkActor )

public:

    using AActor::AActor;

    void Receive ( const FMessage &InMessage ) noexcept override
    {
        const TOptional<FActorBenchmarkPayload> Payload = InMessage.TryGetPayload<FActorBenchmarkPayload>();
        if ( Payload.HasValue() )
        {
            ReceivedSequenceSum += ( *Payload ).Sequence;
        }
    }

public:

    UInt64 ReceivedSequenceSum{ 0ULL };
};

/// @brief Creates the same payload message for every benchmark iteration.
[[nodiscard]] inline FMessage MakeActorBenchmarkMessage ( const UInt64 InSequence = 0ULL ) noexcept
{
    return FMessage::Make( ActorID{}, FActorBenchmarkPayload{ .Sequence = InSequence } );
}

/// @brief Drains a mailbox without timing it, used to restore a known state between measured operations.
inline void DrainMailbox ( FMailBox &InMailbox ) noexcept
{
    for ( ;; )
    {
        TOptional<FMessage> Message = InMailbox.Pop();
        if ( not Message.HasValue() )
        {
            break;
        }

        Benchmarks::DoNotOptimize( Message );
    }
}

} // namespace Lumen
