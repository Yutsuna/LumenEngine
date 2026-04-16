/**
 * @file ActorTypes.hpp
 * @brief Actor types definition for parallel processing in Lumen Engine.
 */

#pragma once

#include "CoreTypes.hpp"
#include "Logging/LoggingCategory.hpp"

namespace LumenEngine
{

using ActorID = UInt64;
struct FMessage;
class AActor;

namespace Concepts
{

    template <typename Type>
    concept CActor = requires( Type InActor, FMessage InMessage ) {
        { InActor.Receive( InMessage ) } -> std::same_as<void>;
    };

} // namespace Concepts

/**
 * @class FActorRef
 * @brief Lightweight handle to an actor.
 * @details Used for communication instead of TSharedPtr to avoid keeping actors alive
 *          indefinitely and to prevent memory cycles.
 *          Currently a wrapper around ActorID, but designed to be extended with
 *          safety checks (e.g. WeakPtr) or remote delivery capabilities.
 */
class FActorRef
{
public:

    FActorRef () noexcept = default;

    explicit FActorRef ( ActorID InId ) noexcept;

    explicit FActorRef ( AActor *InActor ) noexcept;

public:

    [[nodiscard]] ActorID GetId () const noexcept;
    [[nodiscard]] Bool IsValid () const noexcept;

    /**
     * @brief Enqueues a message to the referenced actor.
     * @param InMessage The message to deliver.
     */
    void EnqueueMessage ( const FMessage &InMessage ) const noexcept;

    /** Comparison Operators */
    auto operator<=>( const FActorRef & ) const = default;

private:

    ActorID Id = 0ULL;
};

extern const FLogCategory LogActor;

} // namespace LumenEngine