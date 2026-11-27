/**
 * @file ActorEmitContext.cpp
 * @brief Per-actor bounded outbox implementation.
 */

#include <Actor/ActorEmitContext.hpp>

void Lumen::FActorEmitContext::Reserve ( const USize InCapacity )
{
    Emissions.reserve( InCapacity );
    ReservedCapacity = InCapacity;
}

Lumen::TExpected<void, Lumen::EActorSendError> Lumen::FActorEmitContext::Emit ( const ActorID InTarget,
                                                                                const FMessage &InMessage ) noexcept
{
    if ( InTarget == InvalidActorID )
    {
        return MakeUnexpected( EActorSendError::InvalidActor );
    }

    if ( Emissions.size() >= ReservedCapacity ) [[unlikely]]
    {
        return MakeUnexpected( EActorSendError::OutboxFull );
    }

    FActorEmission Emission{ .Target = InTarget, .Sender = Sender, .SenderSequence = NextSenderSequence, .Message = InMessage };
    Emission.Message.Sender = Sender;
    ++NextSenderSequence;
    Emissions.push_back( Emission );
    return {};
}

Lumen::TSpan<const Lumen::FActorEmission> Lumen::FActorEmitContext::GetEmissions () const noexcept
{
    return TSpan<const FActorEmission>( Emissions.data(), Emissions.size() );
}

Lumen::UInt64 Lumen::FActorEmitContext::GetFrameIndex () const noexcept
{
    return FrameIndex;
}

void Lumen::FActorEmitContext::Begin ( const ActorID InSender, const UInt64 InFrameIndex ) noexcept
{
    Emissions.clear();
    Sender              = InSender;
    FrameIndex          = InFrameIndex;
    NextSenderSequence  = 0ULL;
    NextCommandSequence = 0ULL;
}

Lumen::UInt64 Lumen::FActorEmitContext::AllocateCommandSequence () noexcept
{
    return NextCommandSequence++;
}
