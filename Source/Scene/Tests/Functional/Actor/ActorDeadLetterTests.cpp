/**
 * @file ActorDeadLetterTests.cpp
 * @brief Tests structured dead-letter reasons and the bounded diagnostic ring.
 */

#include <Actor/ActorLifecycle.hpp>

#include <Actor/ActorMessage.hpp>
#include <Actor/ActorRegistry.hpp>
#include <Actor/ActorTypes.hpp>
#include <Templates/Vector.hpp>
#include <Tests/UnitTest.hpp>

namespace Lumen
{

namespace
{

    constexpr ActorID DiagnosticTarget = 1ULL;

    constexpr EActorSendError AllReasons[] = {
        EActorSendError::InvalidActor,      EActorSendError::UnknownActor, EActorSendError::StaleActor,
        EActorSendError::ActorRetiring,     EActorSendError::MailboxFull,  EActorSendError::OutboxFull,
        EActorSendError::DeferredQueueFull, EActorSendError::WorldClosed,
    };

} // namespace

LUMEN_TEST( FActorRegistry_DeadLetters, ReasonsAreCountedIndependently )
{
    UInt64 TotalBefore                                                   = Internal::FActorRegistry::GetDeadLetterCount();
    UInt64 PerReasonBefore[static_cast<USize>( EActorSendError::Count )] = {};

    for ( USize ReasonIndex = 0ULL; ReasonIndex < static_cast<USize>( EActorSendError::Count ); ++ReasonIndex )
    {
        PerReasonBefore[ReasonIndex] =
            Internal::FActorRegistry::GetDeadLetterCount( static_cast<EActorSendError>( ReasonIndex ) );
    }

    const FMessage Message = FMessage::Make( 79U, 0ULL );
    for ( const EActorSendError Reason : AllReasons )
    {
        Internal::FActorRegistry::RecordDeadLetter( Reason, DiagnosticTarget, Message, 7ULL, 11ULL );
    }

    LUMEN_CHECK_EQ( Internal::FActorRegistry::GetDeadLetterCount() - TotalBefore, 8ULL );

    for ( const EActorSendError Reason : AllReasons )
    {
        const USize ReasonIndex = static_cast<USize>( Reason );
        LUMEN_CHECK_EQ( Internal::FActorRegistry::GetDeadLetterCount( Reason ) - PerReasonBefore[ReasonIndex], 1ULL );
    }

    TVector<FDeadLetterRecord> Records;
    Records.resize( 8ULL );
    const USize Copied = Internal::FActorRegistry::ReadDeadLetters( TSpan<FDeadLetterRecord>( Records.data(), Records.size() ) );
    LUMEN_CHECK_EQ( Copied, 8ULL );

    for ( USize Index = 0ULL; Index < Copied; ++Index )
    {
        LUMEN_CHECK_EQ( Records[Index].Reason, AllReasons[Index] );
        LUMEN_CHECK_EQ( Records[Index].Target, DiagnosticTarget );
        LUMEN_CHECK_EQ( Records[Index].FrameIndex, 7ULL );
        LUMEN_CHECK_EQ( Records[Index].SenderSequence, 11ULL );
    }
}

LUMEN_TEST( FActorRegistry_DeadLetters, InvalidAndUnknownTargetsUseDistinctReasons )
{
    const UInt64 InvalidBefore = Internal::FActorRegistry::GetDeadLetterCount( EActorSendError::InvalidActor );
    const UInt64 UnknownBefore = Internal::FActorRegistry::GetDeadLetterCount( EActorSendError::UnknownActor );

    const FActorRef Invalid;
    const TExpected<void, EActorSendError> InvalidResult = Invalid.TryEnqueueMessage( FMessage::Make( 80U, 0ULL ) );
    LUMEN_CHECK_HAS_ERROR( InvalidResult );
    LUMEN_CHECK_EQ( InvalidResult.Error(), EActorSendError::InvalidActor );

    /** INFO: Slot 0x0200'0000 is outside the allocated chunk range. */
    constexpr ActorID UnresolvedId = 0x0000'0000'0200'0000ULL;
    const FActorRef Unknown( UnresolvedId );
    const TExpected<void, EActorSendError> UnknownResult = Unknown.TryEnqueueMessage( FMessage::Make( 81U, 0ULL ) );
    LUMEN_CHECK_HAS_ERROR( UnknownResult );
    LUMEN_CHECK_EQ( UnknownResult.Error(), EActorSendError::UnknownActor );

    LUMEN_CHECK_EQ( Internal::FActorRegistry::GetDeadLetterCount( EActorSendError::InvalidActor ) - InvalidBefore, 0ULL );
    LUMEN_CHECK_EQ( Internal::FActorRegistry::GetDeadLetterCount( EActorSendError::UnknownActor ) - UnknownBefore, 1ULL );
}

LUMEN_TEST( FActorRegistry_DeadLetters, DiagnosticRingSaturatesWithoutUnboundedGrowth )
{
    const UInt64 TotalBefore   = Internal::FActorRegistry::GetDeadLetterCount();
    const UInt64 DroppedBefore = Internal::FActorRegistry::GetDroppedDeadLetterRecordCount();

    constexpr UInt32 Burst = 300U;
    const FMessage Message = FMessage::Make( 82U, 0ULL );
    for ( UInt32 Index = 0U; Index < Burst; ++Index )
    {
        Internal::FActorRegistry::RecordDeadLetter( EActorSendError::MailboxFull, DiagnosticTarget, Message, 9ULL,
                                                    static_cast<UInt64>( Index ) );
    }

    const UInt64 TotalAfter = Internal::FActorRegistry::GetDeadLetterCount();
    LUMEN_CHECK_EQ( TotalAfter - TotalBefore, static_cast<UInt64>( Burst ) );

    LUMEN_CHECK_TRUE( Internal::FActorRegistry::GetDeadLetterRecordCount() <= Internal::FActorRegistry::DeadLetterCapacity );

    const UInt64 ExpectedDroppedAfter  = TotalAfter > Internal::FActorRegistry::DeadLetterCapacity
                                             ? TotalAfter - Internal::FActorRegistry::DeadLetterCapacity
                                             : 0ULL;
    const UInt64 ExpectedDroppedBefore = TotalBefore > Internal::FActorRegistry::DeadLetterCapacity
                                             ? TotalBefore - Internal::FActorRegistry::DeadLetterCapacity
                                             : 0ULL;

    LUMEN_CHECK_EQ( Internal::FActorRegistry::GetDroppedDeadLetterRecordCount() - DroppedBefore,
                    ExpectedDroppedAfter - ExpectedDroppedBefore );

    TVector<FDeadLetterRecord> Records;
    Records.resize( Internal::FActorRegistry::DeadLetterCapacity );
    const USize Copied = Internal::FActorRegistry::ReadDeadLetters( TSpan<FDeadLetterRecord>( Records.data(), Records.size() ) );
    LUMEN_CHECK_EQ( Copied, Internal::FActorRegistry::DeadLetterCapacity );

    for ( USize Index = 0ULL; Index < Copied; ++Index )
    {
        LUMEN_CHECK_EQ( Records[Index].Reason, EActorSendError::MailboxFull );
    }

    /** INFO: The newest retained record is the last one written. */
    LUMEN_CHECK_EQ( Records[Copied - 1ULL].SenderSequence, static_cast<UInt64>( Burst - 1U ) );
}

} // namespace Lumen
