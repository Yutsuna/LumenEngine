/**
 * @file ActorTypes.hpp
 * @brief Actor types definition.
 */

#pragma once

#include <SceneExport.hpp>

#include <Core/CoreTypes.hpp>

#include <Concepts/SameAs.hpp>
#include <Concepts/TriviallyCopyable.hpp>

#include <Templates/Hash.hpp>

#include <Logging/LoggingCategory.hpp>

namespace Lumen
{

/**
 * @typedef ActorID
 * @brief A unique identifier for an actor.
 */
using ActorID = UInt64;

/**
 * @brief The invalid actor ID value.
 */
inline constexpr ActorID InvalidActorID = 0ULL;

/**
 * @brief Computes the slot index from an actor ID.
 */
[[nodiscard]] constexpr UInt32 ActorIdSlotIndex ( ActorID InId ) noexcept;

/**
 * @brief Computes the generation from an actor ID.
 */
[[nodiscard]] constexpr UInt32 ActorIdGeneration ( ActorID InId ) noexcept;

/**
 * @brief Constructs an actor ID from a slot index and generation.
 */
[[nodiscard]] constexpr ActorID MakeActorId ( UInt32 InSlotIndex, UInt32 InGeneration ) noexcept;

struct FMessage;
class AActor;

namespace Concepts
{

    /**
     * @concept CActor
     * @brief Concept for an actor type that can receive messages.
     */
    template <typename Type>
    concept CActor = requires( Type InActor, FMessage InMessage ) {
        { InActor.Receive( InMessage ) } -> CSameAs<void>;
    };

    /**
     * @concept CTriviallyCopyablePayload
     * @brief Ensures that a type is safe to be sent as a message payload.
     * @details Requires the type to be trivially copyable, meaning it has no
     *          non-trivial destructor, virtual functions, or complex state that
     *          cannot be bitwise copied.
     */
    template <typename Type>
    concept CTriviallyCopyablePayload = Concepts::CTriviallyCopyable<Type>;

} // namespace Concepts

namespace Internal
{

    /**
     * @trait TActorTypeRegistry
     * @brief Compile-time registry for actor types, associating each type with a unique ID.
     */
    template <typename TActorType> struct TActorTypeRegistry
    {
        static constexpr USize ID = static_cast<USize>( FHash::TypeHash<TActorType>() );
    };

} // namespace Internal

#define LUMEN_ACTOR_BODY( ActorClassName )                                                                                       \
                                                                                                                                 \
public:                                                                                                                          \
                                                                                                                                 \
    [[nodiscard]] inline ::Lumen::USize GetTypeID() const noexcept override                                                      \
    {                                                                                                                            \
        return ::Lumen::Internal::TActorTypeRegistry<ActorClassName>::ID;                                                        \
    }

/**
 * @class FActorRef
 * @brief Lightweight handle to an actor.
 * @details Used for communication instead of TSharedPtr to avoid keeping actors alive
 *          indefinitely and to prevent memory cycles.
 * @note Use FActorRef for cross-actor object references inside message payloads instead of raw pointers.
 */
class SCENE_EXPORT FActorRef
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

} // namespace Lumen

#include "Inline/ActorTypes.inl"
