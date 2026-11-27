/**
 * @file ActorLifecycle.hpp
 * @brief Actor lifecycle, tick and structured error contracts.
 */

#pragma once

#include <Core/CoreTypes.hpp>
#include <SceneExport.hpp>

namespace Lumen
{

/** @enum EActorLifecycle
 * @brief Logical actor lifetime, independent from its C++ storage lifetime. */
enum class EActorLifecycle : UInt8
{
    Spawning,
    Active,
    PendingKill,
    Retired,
};

/** @enum EActorTickPhase
 * @brief Explicit world dispatch phase selected by an actor. */
enum class EActorTickPhase : UInt8
{
    FixedUpdate,
    PreUpdate,
    Update,
    PostUpdate,
};

/** @enum EActorSendError
 * @brief Structured actor routing and dead-letter reasons. */
enum class EActorSendError : UInt8
{
    InvalidActor,
    UnknownActor,
    StaleActor,
    ActorRetiring,
    MailboxFull,
    OutboxFull,
    DeferredQueueFull,
    WorldClosed,
    Count,
};

/** @enum EActorSpawnError
 * @brief Structured failures accepted by fallible spawn operations. */
enum class EActorSpawnError : UInt8
{
    RegistryExhausted,
    RegistrationFailed,
    WorldCapacityExceeded,
    DeferredQueueFull,
    WorldClosed,
};

/** @enum EActorDestroyError
 * @brief Structured failures accepted by fallible destroy operations. */
enum class EActorDestroyError : UInt8
{
    InvalidActor,
    UnknownActor,
    ActorRetiring,
    DeferredQueueFull,
    WorldClosed,
};

} // namespace Lumen
