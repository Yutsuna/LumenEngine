/**
 * @file WorldTickDeterminismTests.cpp
 * @brief Tests explicit phases, fixed steps and bounded deterministic dispatch.
 */

#include <World/World.hpp>
#include <World/WorldTypes.hpp>

#include <Actor/Actor.hpp>
#include <Actor/ActorMessage.hpp>
#include <Actor/ActorTypes.hpp>

#include <Templates/Vector.hpp>
#include <Tests/UnitTest.hpp>

namespace Lumen
{

namespace
{

    /** @struct FPhaseTrace
     * @brief Atomic phase tickets recorded by one actor per phase.
     */
    struct FPhaseTrace
    {
        TAtomic<UInt32> NextTicket  = { 0U };
        TAtomic<UInt32> FixedOrder  = { 0U };
        TAtomic<UInt32> PreOrder    = { 0U };
        TAtomic<UInt32> UpdateOrder = { 0U };
        TAtomic<UInt32> PostOrder   = { 0U };
    };

    /** @class FPhaseActor
     * @brief Records its dispatch ticket for one explicit phase.
     */
    class FPhaseActor final : public AActor
    {
        LUMEN_ACTOR_BODY ( FPhaseActor );

    public:

        FPhaseActor ( const ActorID InId, const EActorTickPhase InPhase, FPhaseTrace *InTrace ) noexcept
            : AActor( InId ), Phase( InPhase ), Trace( InTrace )
        {
        }

        void Receive ( const FMessage & ) noexcept override
        {
        }

        void OnFixedTick ( const Float64 ) noexcept override
        {
            Record();
        }

        void OnTick ( const Float64 ) noexcept override
        {
            Record();
        }

        [[nodiscard]] EActorTickPhase GetTickPhase () const noexcept override
        {
            return Phase;
        }

    private:

        void Record () noexcept
        {
            const UInt32 Ticket = Trace->NextTicket.fetch_add( 1U, std::memory_order_relaxed ) + 1U;
            switch ( Phase )
            {
            case EActorTickPhase::FixedUpdate:
                Trace->FixedOrder.store( Ticket, std::memory_order_release );
                break;
            case EActorTickPhase::PreUpdate:
                Trace->PreOrder.store( Ticket, std::memory_order_release );
                break;
            case EActorTickPhase::Update:
                Trace->UpdateOrder.store( Ticket, std::memory_order_release );
                break;
            case EActorTickPhase::PostUpdate:
                Trace->PostOrder.store( Ticket, std::memory_order_release );
                break;
            }
        }

        EActorTickPhase Phase;
        FPhaseTrace *Trace = nullptr;
    };

    /** @class FTickCountActor
     * @brief Counts its own dispatched tasks and received messages.
     */
    class FTickCountActor final : public AActor
    {
        LUMEN_ACTOR_BODY ( FTickCountActor );

    public:

        explicit FTickCountActor ( const ActorID InId ) noexcept : AActor( InId )
        {
        }

        void Receive ( const FMessage & ) noexcept override
        {
            ReceiveCount.fetch_add( 1U, std::memory_order_relaxed );
        }

        void OnTick ( const Float64 ) noexcept override
        {
            TickCount.fetch_add( 1U, std::memory_order_relaxed );
        }

        [[nodiscard]] UInt32 GetReceiveCount () const noexcept
        {
            return ReceiveCount.load( std::memory_order_relaxed );
        }

        [[nodiscard]] UInt32 GetTickCount () const noexcept
        {
            return TickCount.load( std::memory_order_relaxed );
        }

        void ReserveMailbox ( const USize InCapacity )
        {
            LUMEN_CHECK_HAS_VALUE( GetMailbox().Reserve( InCapacity ) );
        }

    private:

        TAtomic<UInt32> ReceiveCount = { 0U };
        TAtomic<UInt32> TickCount    = { 0U };
    };

    /** @class FEmittingActor
     * @brief Emits a bounded burst to a target from inside its task.
     */
    class FEmittingActor final : public AActor
    {
        LUMEN_ACTOR_BODY ( FEmittingActor );

    public:

        FEmittingActor ( const ActorID InId, const ActorID InTarget, const UInt32 InBurst ) noexcept
            : AActor( InId ), Target( InTarget ), Burst( InBurst )
        {
        }

        void Receive ( const FMessage & ) noexcept override
        {
        }

        void OnTick ( const Float64 ) noexcept override
        {
            for ( UInt32 Index = 0U; Index < Burst; ++Index )
            {
                const FMessage Message                  = FMessage::Make( MessageType, GetId(), static_cast<UInt32>( GetId() ) );
                TExpected<void, EActorSendError> Result = TrySendMessage( Target, Message );
                if ( Result )
                {
                    EmittedCount.fetch_add( 1U, std::memory_order_relaxed );
                }
                else
                {
                    LastError.store( static_cast<UInt32>( Result.Error() ), std::memory_order_release );
                }
            }
        }

        [[nodiscard]] UInt32 GetEmittedCount () const noexcept
        {
            return EmittedCount.load( std::memory_order_relaxed );
        }

        [[nodiscard]] UInt32 GetLastError () const noexcept
        {
            return LastError.load( std::memory_order_relaxed );
        }

        static constexpr UInt32 MessageType = 71U;

    private:

        ActorID Target = InvalidActorID;
        UInt32 Burst   = 0U;

        TAtomic<UInt32> EmittedCount = { 0U };
        TAtomic<UInt32> LastError    = { 0U };
    };

    /** @class FCollectingActor
     * @brief Appends received payloads to a single-consumer trace.
     */
    class FCollectingActor final : public AActor
    {
        LUMEN_ACTOR_BODY ( FCollectingActor );

    public:

        FCollectingActor ( const ActorID InId, TVector<UInt32> *InTrace ) noexcept : AActor( InId ), Trace( InTrace )
        {
        }

        void Receive ( const FMessage &InMessage ) noexcept override
        {
            Trace->push_back( InMessage.GetPayload<UInt32>() );
        }

    private:

        TVector<UInt32> *Trace = nullptr;
    };

} // namespace

LUMEN_TEST( FWorld_DeterministicTick, PhasesExecuteInDeclaredOrder )
{
    FWorldDesc Desc;
    Desc.WorkerCount    = 2U;
    Desc.FixedDeltaTime = 0.1;

    FWorld World( Desc );
    FPhaseTrace Trace;

    auto Fixed  = World.SpawnActor<FPhaseActor>( EActorTickPhase::FixedUpdate, &Trace );
    auto Pre    = World.SpawnActor<FPhaseActor>( EActorTickPhase::PreUpdate, &Trace );
    auto Update = World.SpawnActor<FPhaseActor>( EActorTickPhase::Update, &Trace );
    auto Post   = World.SpawnActor<FPhaseActor>( EActorTickPhase::PostUpdate, &Trace );

    const TExpected<FWorldTickResult, EWorldTickError> Result = World.TryTick( 0.1 );

    LUMEN_CHECK_HAS_VALUE( Result );
    LUMEN_CHECK_EQ( Fixed->GetId() != InvalidActorID, true );
    LUMEN_CHECK_EQ( Pre->GetId() != InvalidActorID, true );
    LUMEN_CHECK_EQ( Update->GetId() != InvalidActorID, true );
    LUMEN_CHECK_EQ( Post->GetId() != InvalidActorID, true );

    /** INFO: One fixed step, then Pre/Update/Post — fenced in exactly that order. */
    LUMEN_CHECK_EQ( Result->FixedStepsExecuted, 1U );
    LUMEN_CHECK_EQ( Result->DispatchedTaskCount, 4ULL );
    LUMEN_CHECK_EQ( Trace.FixedOrder.load( std::memory_order_acquire ), 1U );
    LUMEN_CHECK_EQ( Trace.PreOrder.load( std::memory_order_acquire ), 2U );
    LUMEN_CHECK_EQ( Trace.UpdateOrder.load( std::memory_order_acquire ), 3U );
    LUMEN_CHECK_EQ( Trace.PostOrder.load( std::memory_order_acquire ), 4U );

    const FWorldPhaseCompletion LastPhase = World.GetLastPhaseCompletion();
    LUMEN_CHECK_EQ( LastPhase.Context.Phase, EActorTickPhase::PostUpdate );
    LUMEN_CHECK_EQ( LastPhase.Context.FrameIndex, 1ULL );
    LUMEN_CHECK_EQ( LastPhase.CompletedTaskCount, 1ULL );
    LUMEN_CHECK_FALSE( LastPhase.bCapacityError );
}

LUMEN_TEST( FWorld_DeterministicTick, FixedStepsFollowAccumulatorAndExplicitStep )
{
    FWorldDesc NoFixedDesc;
    NoFixedDesc.WorkerCount = 1U;

    FWorld NoFixedWorld( NoFixedDesc );
    FPhaseTrace NoFixedTrace;
    auto NoFixedActor = NoFixedWorld.SpawnActor<FPhaseActor>( EActorTickPhase::FixedUpdate, &NoFixedTrace );

    const TExpected<FWorldTickResult, EWorldTickError> NoFixed = NoFixedWorld.TryTick( 0.016 );
    LUMEN_CHECK_HAS_VALUE( NoFixed );
    LUMEN_CHECK_EQ( NoFixed->FixedStepsExecuted, 0U );
    LUMEN_CHECK_EQ( NoFixed->DispatchedTaskCount, 0ULL );
    LUMEN_CHECK_EQ( NoFixedTrace.FixedOrder.load( std::memory_order_acquire ), 0U );

    const TExpected<FWorldTickResult, EWorldTickError> Explicit = NoFixedWorld.TryFixedTick( 0.016 );
    LUMEN_CHECK_HAS_VALUE( Explicit );
    LUMEN_CHECK_EQ( Explicit->FixedStepsExecuted, 1U );
    LUMEN_CHECK_EQ( Explicit->DispatchedTaskCount, 1ULL );
    LUMEN_CHECK_EQ( NoFixedTrace.FixedOrder.load( std::memory_order_acquire ), 1U );
    LUMEN_CHECK_EQ( NoFixedWorld.GetLastPhaseCompletion().Context.Phase, EActorTickPhase::FixedUpdate );

    FWorldDesc AccumulatingDesc;
    AccumulatingDesc.WorkerCount    = 1U;
    AccumulatingDesc.FixedDeltaTime = 0.1;

    FWorld AccumulatingWorld( AccumulatingDesc );
    FPhaseTrace AccumulatingTrace;
    auto AccumulatingActor = AccumulatingWorld.SpawnActor<FPhaseActor>( EActorTickPhase::FixedUpdate, &AccumulatingTrace );

    const TExpected<FWorldTickResult, EWorldTickError> TwoSteps = AccumulatingWorld.TryTick( 0.25 );
    LUMEN_CHECK_HAS_VALUE( TwoSteps );
    LUMEN_CHECK_EQ( TwoSteps->FixedStepsExecuted, 2U );
    LUMEN_CHECK_EQ( TwoSteps->DispatchedTaskCount, 2ULL );
    LUMEN_CHECK_EQ( AccumulatingTrace.NextTicket.load( std::memory_order_acquire ), 2U );
}

LUMEN_TEST( FWorld_DeterministicTick, OneTaskPerActorPerPhase )
{
    FWorldDesc Desc;
    Desc.WorkerCount = 4U;

    FWorld World( Desc );
    auto First  = World.SpawnActor<FTickCountActor>();
    auto Second = World.SpawnActor<FTickCountActor>();
    auto Third  = World.SpawnActor<FTickCountActor>();
    auto Fourth = World.SpawnActor<FTickCountActor>();

    const TExpected<FWorldTickResult, EWorldTickError> Result = World.TryTick( 0.0 );
    LUMEN_CHECK_HAS_VALUE( Result );
    LUMEN_CHECK_EQ( Result->DispatchedTaskCount, 4ULL );

    LUMEN_CHECK_EQ( First->GetTickCount(), 1U );
    LUMEN_CHECK_EQ( Second->GetTickCount(), 1U );
    LUMEN_CHECK_EQ( Third->GetTickCount(), 1U );
    LUMEN_CHECK_EQ( Fourth->GetTickCount(), 1U );
}

LUMEN_TEST( FWorld_DeterministicTick, EmittedMessagesAreConsumedOnNextTick )
{
    FWorld World;
    TVector<UInt32> Received;

    auto Receiver = World.SpawnActor<FCollectingActor>( &Received );
    auto Sender   = World.SpawnActor<FEmittingActor>( Receiver->GetId(), 1U );

    const TExpected<FWorldTickResult, EWorldTickError> EmitTick = World.TryTick( 0.0 );
    LUMEN_CHECK_HAS_VALUE( EmitTick );
    LUMEN_CHECK_EQ( Sender->GetEmittedCount(), 1U );
    LUMEN_CHECK_EQ( Sender->GetLastError(), 0U );
    LUMEN_CHECK_EQ( Received.size(), 0ULL );

    const TExpected<FWorldTickResult, EWorldTickError> ConsumeTick = World.TryTick( 0.0 );
    LUMEN_CHECK_HAS_VALUE( ConsumeTick );
    LUMEN_CHECK_EQ( EmitTick->RoutedMessageCount, 1ULL );
    LUMEN_CHECK_EQ( Received.size(), 1ULL );
    LUMEN_CHECK_EQ( Received.front(), static_cast<UInt32>( Sender->GetId() ) );
}

LUMEN_TEST( FWorld_DeterministicTick, MessageBudgetCapsWorkPerTick )
{
    FWorld World;
    auto Receiver          = World.SpawnActor<FTickCountActor>();
    const FActorRef Sender = Receiver->GetRef();

    constexpr UInt32 MessageCount = 150U;
    Receiver->ReserveMailbox( static_cast<USize>( MessageCount ) );

    for ( UInt32 Index = 0U; Index < MessageCount; ++Index )
    {
        Sender.EnqueueMessage( FMessage::Make( 72U, 0ULL ) );
    }

    const TExpected<FWorldTickResult, EWorldTickError> FirstTick = World.TryTick( 0.0 );
    LUMEN_CHECK_HAS_VALUE( FirstTick );
    LUMEN_CHECK_EQ( Receiver->GetReceiveCount(), FWorld::MaxMessagesPerTick );

    const TExpected<FWorldTickResult, EWorldTickError> SecondTick = World.TryTick( 0.0 );
    LUMEN_CHECK_HAS_VALUE( SecondTick );
    LUMEN_CHECK_EQ( Receiver->GetReceiveCount(), MessageCount );
}

} // namespace Lumen
