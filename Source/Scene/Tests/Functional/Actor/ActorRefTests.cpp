/*
 * @file ActorRefTests.cpp
 * @brief Functional tests for FActorRef.
 */

#include <Actor/Actor.hpp>
#include <Actor/ActorMessage.hpp>
#include <Actor/ActorTypes.hpp>
#include <Templates/Vector.hpp>
#include <Tests/UnitTest.hpp>

namespace Lumen
{

/**
 * Test actor fixture
 */

/** @brief Minimal actor recording received messages. */
class FRefProbeActor final : public AActor
{
    LUMEN_ACTOR_BODY ( FRefProbeActor );

public:

    explicit FRefProbeActor ( const ActorID InId ) noexcept : AActor( InId )
    {
        /**
         *
         */
    }

    void Receive ( const FMessage &InMessage ) noexcept override
    {
        ReceivedTypes.push_back( InMessage.Type );
    }

    [[nodiscard]] const TVector<UInt32> &GetReceivedTypes () const noexcept
    {
        return ReceivedTypes;
    }

private:

    TVector<UInt32> ReceivedTypes;
};

/**
 * Construction
 */

LUMEN_TEST( FActorRef_Construct, DefaultRefIsInvalid )
{
    constexpr ActorID DefaultId = 0ULL;

    const FActorRef Ref;

    LUMEN_CHECK_EQ( Ref.GetId(), DefaultId );
    LUMEN_CHECK_FALSE( Ref.IsValid() );
}

LUMEN_TEST( FActorRef_Construct, FromExplicitIdIsValid )
{
    constexpr ActorID ActorId = 3001ULL;

    const FActorRef Ref( ActorId );

    LUMEN_CHECK_EQ( Ref.GetId(), ActorId );
    LUMEN_CHECK_TRUE( Ref.IsValid() );
}

LUMEN_TEST( FActorRef_Construct, FromNullActorIsInvalid )
{
    const FActorRef Ref( static_cast<AActor *>( nullptr ) );

    LUMEN_CHECK_EQ( Ref.GetId(), 0ULL );
    LUMEN_CHECK_FALSE( Ref.IsValid() );
}

LUMEN_TEST( FActorRef_Construct, FromLiveActorCapturesItsId )
{
    FRefProbeActor Actor( 3002ULL );

    const FActorRef Ref( &Actor );

    LUMEN_CHECK_EQ( Ref.GetId(), 3002ULL );
    LUMEN_CHECK_TRUE( Ref.IsValid() );
}

/**
 * Comparison
 */

LUMEN_TEST( FActorRef_Compare, OrdersAndEquatesById )
{
    const FActorRef Empty  = FActorRef{};
    const FActorRef Lower  = FActorRef( 100ULL );
    const FActorRef Higher = FActorRef( 200ULL );

    LUMEN_CHECK_TRUE( Lower == FActorRef( 100ULL ) );
    LUMEN_CHECK_FALSE( Lower == Higher );
    LUMEN_CHECK_TRUE( Lower != Higher );
    LUMEN_CHECK_TRUE( Lower < Higher );
    LUMEN_CHECK_TRUE( Higher > Lower );
    LUMEN_CHECK_TRUE( Empty != Lower );
}

LUMEN_TEST( FActorRef_Copy, KeepsCopiedIdentity )
{
    const FActorRef Source( 3004ULL );
    const FActorRef Copy = Source;

    LUMEN_CHECK_EQ( Copy.GetId(), Source.GetId() );
    LUMEN_CHECK_TRUE( Copy == Source );
}

/**
 * Routing
 */

LUMEN_TEST( FActorRef_Routing, DeliversOnlyToTheTargetActor )
{
    FRefProbeActor TargetActor( 3010ULL );
    FRefProbeActor OtherActor( 3011ULL );

    const FActorRef TargetRef( &TargetActor );

    TargetRef.EnqueueMessage( FMessage::Make( 5U, 0ULL ) );
    TargetRef.EnqueueMessage( FMessage::Make( 6U, 0ULL ) );

    OtherActor.RunFrame( 0.0, 0U );

    LUMEN_CHECK_EQ( OtherActor.GetReceivedTypes().size(), 0ULL );

    TargetActor.RunFrame( 0.0, 0U );

    const TVector<UInt32> &Received = TargetActor.GetReceivedTypes();

    LUMEN_CHECK_EQ( Received.size(), 2ULL );
    LUMEN_CHECK_EQ( Received[0], 5U );
    LUMEN_CHECK_EQ( Received[1], 6U );
}

LUMEN_TEST( FActorRef_Routing, UnknownIdIsSilentlyDiscarded )
{
    constexpr ActorID UnknownId = 3999ULL;

    FRefProbeActor BystanderActor( 3012ULL );
    const FActorRef UnknownRef( UnknownId );

    UnknownRef.EnqueueMessage( FMessage::Make( 7U, 0ULL ) );

    BystanderActor.RunFrame( 0.0, 0U );

    LUMEN_CHECK_EQ( BystanderActor.GetReceivedTypes().size(), 0ULL );
    LUMEN_CHECK_TRUE( UnknownRef.IsValid() );
}

LUMEN_TEST( FActorRef_Routing, DeadReferenceDropsMessages )
{
    /**
     * INFO: FActorRef does not extend the referenced actor lifetime; once the
     *       actor is destroyed, delivering to it is a harmless no-op.
     */
    constexpr ActorID ActorId = 3013ULL;

    FActorRef DyingRef;

    {
        FRefProbeActor DyingActor( ActorId );

        DyingRef = DyingActor.GetRef();

        DyingRef.EnqueueMessage( FMessage::Make( 1U, 0ULL ) );
        DyingActor.RunFrame( 0.0, 0U );

        LUMEN_CHECK_EQ( DyingActor.GetReceivedTypes().size(), 1ULL );
    }

    LUMEN_CHECK_TRUE( DyingRef.IsValid() );

    DyingRef.EnqueueMessage( FMessage::Make( 2U, 0ULL ) );

    FRefProbeActor BystanderActor( 3014ULL );

    BystanderActor.RunFrame( 0.0, 0U );

    LUMEN_CHECK_EQ( BystanderActor.GetReceivedTypes().size(), 0ULL );
}

} // namespace Lumen
