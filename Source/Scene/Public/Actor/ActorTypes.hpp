///
/// @file ActorTypes.hpp
/// @brief Actor types definition for parallel processing in Lumen Engine.
///

#pragma once

#include <Core/CoreTypes.hpp>
#include <SceneExport.hpp>

#include <Concepts/TriviallyCopyable.hpp>

#include <Logging/LoggingCategory.hpp>

#include <concepts>

namespace Lumen
{

struct FActorID
{
    UInt32 Index      = 0U;
    UInt32 Generation = 0U;

    [[nodiscard]] constexpr Bool IsValid () const noexcept
    {
        return Index != 0U && Generation != 0U;
    }

    auto operator<=>( const FActorID & ) const = default;
};

using ActorID = FActorID;
struct FMessage;
class AActor;

/// @brief Opt-in payload traits; enabling a type is an assertion by its author, not a compiler-verified guarantee.
/// @details Any storage referenced by an enabled payload must remain valid through delivery. When Type and Version are
/// provided, their pair must identify one unique payload schema. Different C++ types may share that identity only when
/// they intentionally represent the same data schema.
template <typename PayloadType> struct TMessageTraits
{
    static constexpr Bool bEnabled = false;
};

enum class EActorRegistrationResult : UInt8
{
    Success,
    InvalidID,
    AlreadyRegistered,
    ActorStopped,
    GenerationExhausted,
};

enum class EEnqueueResult : UInt8
{
    Success,
    Full,
    AllocationFailure,
    ActorUnavailable,
    Closed,
};

namespace Concepts
{

    ///
    /// @concept CActor
    /// @brief Ensures that the object is an Actor with non-throwing message processing.
    ///
    template <typename Type>
    concept CActor = requires( Type InActor, FMessage InMessage ) {
        { InActor.Receive( InMessage ) } -> std::same_as<void>;
        requires noexcept( InActor.Receive( InMessage ) );
    };

    template <typename Type>
    concept CTriviallyCopyablePayload = Concepts::CTriviallyCopyable<Type>;

    template <typename Type>
    concept CMessagePayload = Concepts::CTriviallyCopyable<Type> and TMessageTraits<Type>::bEnabled;

    template <typename Type>
    concept CIdentifiedMessagePayload = CMessagePayload<Type> and requires {
        { TMessageTraits<Type>::Type } -> std::convertible_to<UInt32>;
        { TMessageTraits<Type>::Version } -> std::convertible_to<UInt16>;
    };

} // namespace Concepts

namespace Internal
{

    ///
    /// @trait TActorTypeRegistry
    /// @brief Compile-time registry for actor types, associating each type with a unique ID.
    ///
    template <typename ActorType> struct TActorTypeRegistry
    {
        static inline char Tag = 0;
    };

} // namespace Internal

#define LUMEN_ACTOR_BODY( ActorClassName )                                                                                       \
                                                                                                                                 \
public:                                                                                                                          \
                                                                                                                                 \
    [[nodiscard]] ::Lumen::USize GetTypeID() const noexcept override                                                             \
    {                                                                                                                            \
        return reinterpret_cast<::Lumen::USize>( &::Lumen::Internal::TActorTypeRegistry<ActorClassName>::Tag );                  \
    }

///
/// @class FActorRef
/// @brief Lightweight handle containing both the actor index and its generation.
/// @details A generation mismatch makes an enqueue fail instead of targeting a replacement actor.
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
    [[nodiscard]] EEnqueueResult EnqueueMessage ( const FMessage &InMessage ) const noexcept;

    /// @brief Comparison Operators
    auto operator<=>( const FActorRef & ) const = default;

private:

    ActorID Id = {};
};

template <> struct TMessageTraits<FActorRef>
{
    static constexpr Bool bEnabled = true;
};

extern SCENE_EXPORT const FLogCategory LogActor;

} // namespace Lumen
