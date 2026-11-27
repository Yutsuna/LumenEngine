/**
 * @file ActorEmitContext.hpp
 * @brief Per-actor bounded outbox used by deterministic world dispatch.
 */

#pragma once

#include <Actor/ActorLifecycle.hpp>
#include <Actor/ActorMessage.hpp>
#include <SceneExport.hpp>

#include <Memory/Span.hpp>
#include <Templates/Expected.hpp>
#include <Templates/Vector.hpp>

namespace Lumen
{

/** @struct FActorEmission
 * @brief Message plus its deterministic routing key. */
struct SCENE_EXPORT FActorEmission
{
    ActorID Target        = 0ULL;
    ActorID Sender        = 0ULL;
    UInt64 SenderSequence = 0ULL;
    FMessage Message;
};

/** @class FActorEmitContext
 * @brief Actor-confined outbox merged only after a world phase fence. */
class SCENE_EXPORT FActorEmitContext final
{
public:

    FActorEmitContext () noexcept = default;

    /** @brief Reserves storage during actor or world control operations. */
    void Reserve ( USize InCapacity );

    /** @brief Appends an emission without growing the reserved outbox. */
    [[nodiscard]] TExpected<void, EActorSendError> Emit ( ActorID InTarget, const FMessage &InMessage ) noexcept;

    /** @brief Returns the emissions waiting for the serial world commit. */
    [[nodiscard]] TSpan<const FActorEmission> GetEmissions () const noexcept;

    /** @brief Returns the frame associated with this outbox. */
    [[nodiscard]] UInt64 GetFrameIndex () const noexcept;

private:

    void Begin ( ActorID InSender, UInt64 InFrameIndex ) noexcept;
    [[nodiscard]] UInt64 AllocateCommandSequence () noexcept;

private:

    TVector<FActorEmission> Emissions;
    ActorID Sender             = 0ULL;
    UInt64 FrameIndex          = 0ULL;
    UInt64 NextSenderSequence  = 0ULL;
    UInt64 NextCommandSequence = 0ULL;
    USize ReservedCapacity     = 0ULL;

    friend class AActor;
    friend class FWorld;
};

} // namespace Lumen
