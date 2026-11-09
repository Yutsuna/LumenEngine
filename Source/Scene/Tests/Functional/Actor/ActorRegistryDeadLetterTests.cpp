/**
 * @file ActorRegistryDeadLetterTests.cpp
 * @brief Tests that the actor registry dead letter counter is incremented correctly.
 */

#include <World/World.hpp>

#include <Actor/Actor.hpp>
#include <Actor/ActorMessage.hpp>
#include <Actor/ActorRegistry.hpp>
#include <Actor/ActorTypes.hpp>
#include <Tests/UnitTest.hpp>

namespace Lumen
{

namespace
{
    class FDeadLetterActor final : public AActor
    {
        LUMEN_ACTOR_BODY ( FDeadLetterActor );

    public:

        explicit FDeadLetterActor ( const ActorID InId ) noexcept : AActor( InId )
        {
        }
        void Receive ( const FMessage & ) noexcept override
        {
        }
    };
} // namespace

LUMEN_TEST( FActorRegistry_DeadLetter, UnresolvedNonInvalidIdIsCounted )
{
    const UInt64 StartCount = Internal::FActorRegistry::GetDeadLetterCount();

    /**
     * INFO: Slot 0x0200'0000 >> 10 = 32'768, which exceeds MaxChunks (16'384).
     *       FindSlot() therefore returns nullptr for this id — the "unresolved"
     *       path. Id 0 is exempt, but this id is not.
     */
    constexpr ActorID UnresolvedId = 0x0000'0000'0200'0000ULL;
    static_assert( UnresolvedId != InvalidActorID );

    const FActorRef Ref( UnresolvedId );
    Ref.EnqueueMessage( FMessage::Make( 1U, 0ULL ) );

    LUMEN_CHECK_EQ( Internal::FActorRegistry::GetDeadLetterCount() - StartCount, 1ULL );
}

LUMEN_TEST( FActorRegistry_DeadLetter, SendToDeadActorIncrementsCounter )
{
    const UInt64 StartCount = Internal::FActorRegistry::GetDeadLetterCount();

    FActorRef Ref;

    {
        FWorld World;
        auto Actor = World.SpawnActor<FDeadLetterActor>();
        Ref        = Actor->GetRef();

        /** Live: no dead letter. */
        Ref.EnqueueMessage( FMessage::Make( 1U, 0ULL ) );
        LUMEN_CHECK_EQ( Internal::FActorRegistry::GetDeadLetterCount() - StartCount, 0ULL );
    }
    /** World and Actor destroyed; ~AActor retired the slot. */

    /** Dead: one dead letter. */
    Ref.EnqueueMessage( FMessage::Make( 2U, 0ULL ) );
    LUMEN_CHECK_EQ( Internal::FActorRegistry::GetDeadLetterCount() - StartCount, 1ULL );

    /** And again: another dead letter. */
    Ref.EnqueueMessage( FMessage::Make( 3U, 0ULL ) );
    LUMEN_CHECK_EQ( Internal::FActorRegistry::GetDeadLetterCount() - StartCount, 2ULL );
}

LUMEN_TEST( FActorRegistry_DeadLetter, InvalidIdIsNotCounted )
{
    const UInt64 StartCount = Internal::FActorRegistry::GetDeadLetterCount();

    const FActorRef Invalid;
    Invalid.EnqueueMessage( FMessage::Make( 1U, 0ULL ) );

    /** Id=0 is not a "dead letter" — it's never a valid target. */
    LUMEN_CHECK_EQ( Internal::FActorRegistry::GetDeadLetterCount() - StartCount, 0ULL );
}

} // namespace Lumen
