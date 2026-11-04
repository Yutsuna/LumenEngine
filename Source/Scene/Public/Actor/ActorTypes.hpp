///
/// @file ActorTypes.hpp
/// @brief Actor types definition for parallel processing in Lumen Engine.
///

#pragma once

#include <Core/CoreTypes.hpp>
#include <SceneExport.hpp>

#include <Concepts/TriviallyCopyable.hpp>

#include <Logging/LoggingCategory.hpp>

namespace Lumen
{

using ActorID = UInt64;
struct FMessage;
class AActor;

namespace Concepts
{

    ///
    /// @concept CActor
    /// @brief Ensures that the object is an Actor
    ///
    template <typename Type>
    concept CActor = requires( Type InActor, FMessage InMessage ) {
        { InActor.Receive( InMessage ) } -> std::same_as<void>;
    };

    ///
    /// @concept CTriviallyCopyablePayload
    /// @brief Ensures that a type is safe to be sent as a message payload.
    ///
    template <typename Type>
    concept CTriviallyCopyablePayload = Concepts::CTriviallyCopyable<Type>;

} // namespace Concepts

namespace Internal
{

    ///
    /// @trait TActorTypeRegistry
    /// @brief Compile-time registry for actor types, associating each type with a unique ID.
    ///
    template <typename ActorType> struct TActorTypeRegistry
    {
        static void GetID () noexcept {};
    };

} // namespace Internal

#define LUMEN_ACTOR_BODY( ActorClassName )                                                                                       \
                                                                                                                                 \
public:                                                                                                                          \
                                                                                                                                 \
    [[nodiscard]] ::Lumen::USize GetTypeID() const noexcept override                                                             \
    {                                                                                                                            \
        return reinterpret_cast<::Lumen::USize>( &::Lumen::Internal::TActorTypeRegistry<ActorClassName>::GetID );                \
    }

///
/// @class FActorRef
/// @brief Lightweight handle to an actor.
/// @details Used for communication instead of TSharedPtr to avoid keeping actors alive
///          indefinitely and to prevent memory cycles.
/// @note Use FActorRef for cross-actor object references inside message payloads instead of raw pointers.
///
class SCENE_EXPORT FActorRef
{
public:

    FActorRef () noexcept = default;

    explicit FActorRef ( ActorID InId ) noexcept;

    explicit FActorRef ( AActor *InActor ) noexcept;

public:

    [[nodiscard]] ActorID GetId () const noexcept;
    [[nodiscard]] Bool IsValid () const noexcept;

    ///
    /// @brief Enqueues a message to the referenced actor.
    /// @param InMessage The message to deliver.
    ///
    void EnqueueMessage ( const FMessage &InMessage ) const noexcept;

    /// @brief Comparison Operators
    auto operator<=>( const FActorRef & ) const = default;

private:

    ActorID Id = 0ULL;
};

extern const FLogCategory LogActor;

} // namespace Lumen
