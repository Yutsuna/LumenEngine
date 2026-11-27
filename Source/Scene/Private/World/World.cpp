/**
 * @file World.cpp
 * @brief Implementation of the FWorld class.
 */

#include <World/World.hpp>
#include <World/WorldExecutor.hpp>
#include <World/WorldSpatialRegistry.hpp>

#include <Algo/Sort.hpp>

#include <Actor/Actor.hpp>
#include <Actor/ActorRegistry.hpp>

#include <Meta/Overloaded.hpp>
#include <Templates/Optional.hpp>

namespace
{

[[nodiscard]] Lumen::Bool SortActiveActors ( const Lumen::TSharedPtr<Lumen::AActor> &InLeft,
                                             const Lumen::TSharedPtr<Lumen::AActor> &InRight ) noexcept
{
    const Lumen::USize LeftType  = InLeft->GetTypeID();
    const Lumen::USize RightType = InRight->GetTypeID();
    return LeftType != RightType ? LeftType < RightType : InLeft->GetId() < InRight->GetId();
}

[[nodiscard]] Lumen::Bool SortEmissions ( const Lumen::FActorEmission &InLeft, const Lumen::FActorEmission &InRight ) noexcept
{
    if ( InLeft.Target != InRight.Target )
    {
        return InLeft.Target < InRight.Target;
    }
    if ( InLeft.Sender != InRight.Sender )
    {
        return InLeft.Sender < InRight.Sender;
    }
    return InLeft.SenderSequence < InRight.SenderSequence;
}

} // namespace

Lumen::FWorld::FWorld () noexcept : FWorld( FWorldDesc{} )
{
}

Lumen::FWorld::FWorld ( const FWorldDesc &InDesc ) noexcept
    : Desc( InDesc ), Registry( FSpatialRegistryCapacities{ .EntityCapacity = InDesc.MaxActors } ),
      Executor( MakeUnique<FWorldExecutor>( InDesc.WorkerCount, InDesc.MaxActors ) )
{
    Actors.reserve( Desc.MaxActors );
    ActiveActors.reserve( Desc.MaxActors );
    DeferredCommands.reserve( Desc.DeferredCommandCapacity );
    DeferredCommandBatch.reserve( Desc.DeferredCommandCapacity );
    PhaseActors.reserve( Desc.MaxActors );
    VariableActors.reserve( Desc.MaxActors );
    MergedEmissions.reserve( Desc.EmissionCapacity );
}

Lumen::FWorld::~FWorld () noexcept
{
    bShuttingDown = true;
    Executor.Reset();
    ShutdownActors();
}

void Lumen::FWorld::Broadcast ( const FMessage &InMessage ) noexcept
{
    static_cast<void>( TryBroadcast( InMessage ) );
}

Lumen::TExpected<void, Lumen::EActorSendError> Lumen::FWorld::TryBroadcast ( const FMessage &InMessage ) noexcept
{
    if ( bShuttingDown )
    {
        return MakeUnexpected( EActorSendError::WorldClosed );
    }

    if ( bInParallelPhase )
    {
        if ( not QueueDeferredCommand( InvalidActorID, FDeferredBroadcastCommand{ .Message = InMessage } ) )
        {
            return MakeUnexpected( EActorSendError::DeferredQueueFull );
        }
        return {};
    }

    TOptional<EActorSendError> FirstError;
    for ( const TSharedPtr<AActor> &Actor : ActiveActors )
    {
        if ( Actor->GetLifecycle() != EActorLifecycle::Active )
        {
            continue;
        }

        TExpected<void, EActorSendError> Result =
            Internal::FActorRegistry::TryEnqueueMessage( Actor->GetId(), InMessage, FrameIndex );
        if ( not Result and not FirstError )
        {
            FirstError.Emplace( Result.Error() );
        }
    }

    if ( FirstError )
    {
        return MakeUnexpected( *FirstError );
    }
    return {};
}

void Lumen::FWorld::DestroyActor ( const ActorID InId ) noexcept
{
    static_cast<void>( TryDestroyActor( InId ) );
}

Lumen::TExpected<void, Lumen::EActorDestroyError> Lumen::FWorld::TryDestroyActor ( const ActorID InId ) noexcept
{
    if ( bShuttingDown )
    {
        return MakeUnexpected( EActorDestroyError::WorldClosed );
    }
    if ( InId == InvalidActorID )
    {
        return MakeUnexpected( EActorDestroyError::InvalidActor );
    }

    if ( bInParallelPhase )
    {
        if ( not QueueDeferredCommand( InId, FDeferredDestroyCommand{ .Target = InId } ) )
        {
            return MakeUnexpected( EActorDestroyError::DeferredQueueFull );
        }
        return {};
    }

    return DestroyActorImpl( InId );
}

void Lumen::FWorld::Tick ( const Float64 InDeltaTime ) noexcept
{
    static_cast<void>( TryTick( InDeltaTime ) );
}

Lumen::TExpected<Lumen::FWorldTickResult, Lumen::EWorldTickError> Lumen::FWorld::TryTick ( const Float64 InDeltaTime ) noexcept
{
    if ( bShuttingDown or not Executor.IsValid() )
    {
        return MakeUnexpected( EWorldTickError::ExecutorUnavailable );
    }

    ++FrameIndex;
    FWorldTickResult Result{ .FrameIndex = FrameIndex, .LastPhase = {} };
    TOptional<EWorldTickError> FirstError;

    if ( ApplyDeferredCommands() )
    {
        FirstError.Emplace( EWorldTickError::WorldCapacityExceeded );
    }
    SortActorsIfNeeded();

    VariableActors.clear();
    for ( const TSharedPtr<AActor> &Actor : ActiveActors )
    {
        if ( Actor->GetLifecycle() == EActorLifecycle::Active and Actor->GetTickPhase() != EActorTickPhase::FixedUpdate )
        {
            VariableActors.push_back( Actor.Get() );
            Actor->BeginTick();
        }
    }

    const Float64 DeltaTime = InDeltaTime > 0.0 ? InDeltaTime : 0.0;
    if ( Desc.FixedDeltaTime > 0.0 )
    {
        FixedAccumulator += DeltaTime;
        const Float64 PendingSteps = FixedAccumulator / Desc.FixedDeltaTime;
        UInt32 StepCount           = 0U;

        if ( PendingSteps > static_cast<Float64>( Desc.MaxFixedStepsPerTick ) )
        {
            StepCount        = Desc.MaxFixedStepsPerTick;
            FixedAccumulator = 0.0;
            if ( not FirstError )
            {
                FirstError.Emplace( EWorldTickError::FixedStepLimitExceeded );
            }
        }
        else
        {
            StepCount = static_cast<UInt32>( PendingSteps );
            FixedAccumulator -= static_cast<Float64>( StepCount ) * Desc.FixedDeltaTime;
        }

        for ( UInt32 StepIndex = 0U; StepIndex < StepCount; ++StepIndex )
        {
            TExpected<void, EWorldTickError> StepResult = RunFixedStep( StepIndex, Desc.FixedDeltaTime, Result );
            if ( not StepResult )
            {
                for ( AActor *Actor : VariableActors )
                {
                    Actor->EndTick();
                }
                return MakeUnexpected( StepResult.Error() );
            }
            ++Result.FixedStepsExecuted;
        }
    }

    constexpr EActorTickPhase VariablePhases[] = {
        EActorTickPhase::PreUpdate,
        EActorTickPhase::Update,
        EActorTickPhase::PostUpdate,
    };

    for ( const EActorTickPhase Phase : VariablePhases )
    {
        PhaseActors.clear();
        for ( AActor *Actor : VariableActors )
        {
            if ( Actor->GetTickPhase() == Phase )
            {
                PhaseActors.push_back( Actor );
            }
        }

        TExpected<FWorldPhaseCompletion, EWorldTickError> Completion = RunPhase(
            TSpan<AActor *const>( PhaseActors.data(), PhaseActors.size() ), Phase, Result.FixedStepsExecuted, DeltaTime );
        if ( not Completion )
        {
            for ( AActor *Actor : VariableActors )
            {
                Actor->EndTick();
            }
            return MakeUnexpected( Completion.Error() );
        }

        Result.DispatchedTaskCount += Completion->CompletedTaskCount;
        LastPhase = *Completion;
    }

    for ( AActor *Actor : VariableActors )
    {
        Actor->EndTick();
    }

    if ( not MergeEmissions( TSpan<AActor *const>( VariableActors.data(), VariableActors.size() ), Result ) and not FirstError )
    {
        FirstError.Emplace( EWorldTickError::EmissionCapacityExceeded );
    }

    Registry.Publish();

    if ( FirstError )
    {
        return MakeUnexpected( *FirstError );
    }
    return Result;
}

Lumen::TExpected<Lumen::FWorldTickResult, Lumen::EWorldTickError>
Lumen::FWorld::TryFixedTick ( const Float64 InDeltaTime ) noexcept
{
    if ( bShuttingDown or not Executor.IsValid() )
    {
        return MakeUnexpected( EWorldTickError::ExecutorUnavailable );
    }

    ++FrameIndex;
    ApplyDeferredCommands();
    SortActorsIfNeeded();

    FWorldTickResult Result{ .FrameIndex = FrameIndex, .FixedStepsExecuted = 1U, .LastPhase = {} };
    TExpected<void, EWorldTickError> StepResult = RunFixedStep( 0ULL, InDeltaTime, Result );
    if ( not StepResult )
    {
        return MakeUnexpected( StepResult.Error() );
    }

    Registry.Publish();
    return Result;
}

Lumen::Bool Lumen::FWorld::IsInParallelPhase () const noexcept
{
    return bInParallelPhase;
}

Lumen::FWorldPhaseCompletion Lumen::FWorld::GetLastPhaseCompletion () const noexcept
{
    return LastPhase;
}

Lumen::FSpatialRegistry &Lumen::FWorld::GetSpatialRegistry () noexcept
{
    return Registry;
}

Lumen::TExpected<void, Lumen::EActorSpawnError> Lumen::FWorld::InsertSpawnedActor ( const TSharedPtr<AActor> &InActor ) noexcept
{
    if ( bShuttingDown )
    {
        return MakeUnexpected( EActorSpawnError::WorldClosed );
    }
    if ( not InActor.IsValid() or not InActor->bRegistered )
    {
        return MakeUnexpected( EActorSpawnError::RegistrationFailed );
    }
    if ( Actors.size() >= Desc.MaxActors )
    {
        return MakeUnexpected( EActorSpawnError::WorldCapacityExceeded );
    }

    const ActorID NewId = InActor->GetId();
    if ( not Actors.emplace( NewId, InActor ).second )
    {
        return MakeUnexpected( EActorSpawnError::RegistrationFailed );
    }

    ActiveActors.push_back( InActor );
    bNeedsSorting = true;
    InActor->BeginPlay();
    return {};
}

Lumen::Bool Lumen::FWorld::QueueDeferredCommand ( const ActorID InTarget, FDeferredCommandPayload &&InPayload ) noexcept
{
    FActorEmitContext *const EmitContext = AActor::GetCurrentEmitContext();

    FDeferredCommand Command;
    Command.Target  = InTarget;
    Command.Payload = std::move( InPayload );
    if ( EmitContext != nullptr )
    {
        Command.Sender         = EmitContext->Sender;
        Command.SenderSequence = EmitContext->AllocateCommandSequence();
    }

    HAL::TUniqueLock<HAL::FPlatformMutex> Lock( DeferredMutex );
    if ( bShuttingDown or DeferredCommands.size() >= Desc.DeferredCommandCapacity )
    {
        return false;
    }

    if ( EmitContext == nullptr )
    {
        Command.Sender         = InvalidActorID;
        Command.SenderSequence = NextExternalCommandSequence++;
    }
    DeferredCommands.push_back( std::move( Command ) );
    return true;
}

Lumen::TExpected<void, Lumen::EActorDestroyError> Lumen::FWorld::DestroyActorImpl ( const ActorID InId ) noexcept
{
    const auto ActorIterator = Actors.find( InId );
    if ( ActorIterator == Actors.end() )
    {
        return MakeUnexpected( EActorDestroyError::UnknownActor );
    }

    TSharedPtr<AActor> Actor = ActorIterator->second;
    if ( Actor->GetLifecycle() != EActorLifecycle::Active )
    {
        return MakeUnexpected( EActorDestroyError::ActorRetiring );
    }

    Actor->MarkPendingKill();
    Registry.UnregisterSpatialEntity( InId );

    Actors.erase( ActorIterator );
    for ( auto ActiveIterator = ActiveActors.begin(); ActiveIterator != ActiveActors.end(); ++ActiveIterator )
    {
        if ( ( *ActiveIterator )->GetId() == InId )
        {
            ActiveActors.erase( ActiveIterator );
            break;
        }
    }

    Actor->EndPlay();
    Actor->Retire();
    bNeedsSorting = true;
    return {};
}

Lumen::TExpected<Lumen::FWorldPhaseCompletion, Lumen::EWorldTickError>
Lumen::FWorld::RunPhase ( const TSpan<AActor *const> InActors,
                          const EActorTickPhase InPhase,
                          const UInt64 InFixedStepIndex,
                          const Float64 InDeltaTime ) noexcept
{
    FWorldFrameContext Context{
        .FrameIndex = FrameIndex, .FixedStepIndex = InFixedStepIndex, .TaskCount = InActors.size(), .Phase = InPhase };

    bInParallelPhase = not InActors.empty();
    TExpected<FWorldPhaseCompletion, EWorldTickError> Completion =
        Executor->ExecutePhase( InActors, Context, InDeltaTime, MaxMessagesPerTick );
    bInParallelPhase = false;
    return Completion;
}

Lumen::TExpected<void, Lumen::EWorldTickError>
Lumen::FWorld::RunFixedStep ( const UInt64 InFixedStepIndex, const Float64 InDeltaTime, FWorldTickResult &OutResult ) noexcept
{
    PhaseActors.clear();
    for ( const TSharedPtr<AActor> &Actor : ActiveActors )
    {
        if ( Actor->GetLifecycle() == EActorLifecycle::Active and Actor->GetTickPhase() == EActorTickPhase::FixedUpdate )
        {
            PhaseActors.push_back( Actor.Get() );
            Actor->BeginTick();
        }
    }

    TExpected<FWorldPhaseCompletion, EWorldTickError> Completion =
        RunPhase( TSpan<AActor *const>( PhaseActors.data(), PhaseActors.size() ), EActorTickPhase::FixedUpdate, InFixedStepIndex,
                  InDeltaTime );

    for ( AActor *Actor : PhaseActors )
    {
        Actor->EndTick();
    }

    if ( not Completion )
    {
        return MakeUnexpected( Completion.Error() );
    }

    OutResult.DispatchedTaskCount += Completion->CompletedTaskCount;
    LastPhase = *Completion;
    if ( not MergeEmissions( TSpan<AActor *const>( PhaseActors.data(), PhaseActors.size() ), OutResult ) )
    {
        return MakeUnexpected( EWorldTickError::EmissionCapacityExceeded );
    }
    return {};
}

Lumen::Bool Lumen::FWorld::MergeEmissions ( const TSpan<AActor *const> InActors, FWorldTickResult &OutResult ) noexcept
{
    MergedEmissions.clear();
    Bool bCapacityExceeded = false;

    for ( AActor *Actor : InActors )
    {
        for ( const FActorEmission &Emission : Actor->EmitContext.GetEmissions() )
        {
            if ( MergedEmissions.size() >= Desc.EmissionCapacity )
            {
                Internal::FActorRegistry::RecordDeadLetter( EActorSendError::OutboxFull, Emission.Target, Emission.Message,
                                                            FrameIndex, Emission.SenderSequence );
                bCapacityExceeded = true;
                continue;
            }
            MergedEmissions.push_back( Emission );
        }
        Actor->EmitContext.Emissions.clear();
    }

    Algo::Sort( MergedEmissions, &SortEmissions );
    for ( const FActorEmission &Emission : MergedEmissions )
    {
        TExpected<void, EActorSendError> Routed =
            Internal::FActorRegistry::TryEnqueueMessage( Emission.Target, Emission.Message, FrameIndex, Emission.SenderSequence );
        if ( Routed )
        {
            ++OutResult.RoutedMessageCount;
        }
    }
    return not bCapacityExceeded;
}

Lumen::Bool Lumen::FWorld::ApplyDeferredCommands () noexcept
{
    DeferredCommandBatch.clear();
    {
        HAL::TUniqueLock<HAL::FPlatformMutex> Lock( DeferredMutex );
        DeferredCommandBatch.swap( DeferredCommands );
    }

    Algo::Sort( DeferredCommandBatch, [] ( const FDeferredCommand &InLeft, const FDeferredCommand &InRight ) noexcept -> Bool
    {
        if ( InLeft.Sender != InRight.Sender )
        {
            return InLeft.Sender < InRight.Sender;
        }
        if ( InLeft.SenderSequence != InRight.SenderSequence )
        {
            return InLeft.SenderSequence < InRight.SenderSequence;
        }
        if ( InLeft.Target != InRight.Target )
        {
            return InLeft.Target < InRight.Target;
        }
        return InLeft.Payload.index() < InRight.Payload.index();
    } );

    Bool bWorldCapacityExceeded = false;
    for ( FDeferredCommand &Command : DeferredCommandBatch )
    {
        std::visit( Meta::FOverloaded{ [this, &bWorldCapacityExceeded] ( FDeferredSpawnCommand &InSpawn ) noexcept
        {
            TExpected<void, EActorSpawnError> Inserted = InsertSpawnedActor( InSpawn.Actor );
            if ( not Inserted and Inserted.Error() == EActorSpawnError::WorldCapacityExceeded )
            {
                bWorldCapacityExceeded = true;
            }
        },
                                       [this] ( FDeferredDestroyCommand &InDestroy ) noexcept
        {
            static_cast<void>( DestroyActorImpl( InDestroy.Target ) );
        },
                                       [this] ( FDeferredBroadcastCommand &InBroadcast ) noexcept
        {
            static_cast<void>( TryBroadcast( InBroadcast.Message ) );
        } },
                    Command.Payload );
    }

    DeferredCommandBatch.clear();
    return bWorldCapacityExceeded;
}

void Lumen::FWorld::SortActorsIfNeeded () noexcept
{
    if ( bNeedsSorting )
    {
        Algo::Sort( ActiveActors, &SortActiveActors );
        bNeedsSorting = false;
    }
}

void Lumen::FWorld::ShutdownActors () noexcept
{
    const auto RetirePendingSpawns = [] ( TVector<FDeferredCommand> &InCommands ) noexcept
    {
        for ( FDeferredCommand &Command : InCommands )
        {
            if ( FDeferredSpawnCommand *const Spawn = std::get_if<FDeferredSpawnCommand>( &Command.Payload ) )
            {
                if ( Spawn->Actor.IsValid() )
                {
                    Spawn->Actor->Retire();
                }
            }
        }
    };

    RetirePendingSpawns( DeferredCommands );
    RetirePendingSpawns( DeferredCommandBatch );

    for ( const TSharedPtr<AActor> &Actor : ActiveActors )
    {
        Registry.UnregisterSpatialEntity( Actor->GetId() );
        Actor->MarkPendingKill();
        Actor->EndPlay();
    }
    for ( const TSharedPtr<AActor> &Actor : ActiveActors )
    {
        Actor->Retire();
    }

    Actors.clear();
    ActiveActors.clear();
    DeferredCommands.clear();
    DeferredCommandBatch.clear();
}
