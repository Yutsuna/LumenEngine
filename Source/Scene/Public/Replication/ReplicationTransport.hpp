/**
 * @file ReplicationTransport.hpp
 * @brief Backend-neutral, in-memory-ready transport boundary for replication frames.
 */

#pragma once

#include <SceneExport.hpp>

#include <Replication/ReplicationTypes.hpp>

#include <Core/CoreTypes.hpp>

#include <Memory/Span.hpp>

#include <Templates/Expected.hpp>

namespace Lumen
{

/**
 * @class IReplicationTransport
 * @brief Polled byte transport injected by composition; no socket or native handle is part of this contract.
 * @details Both endpoints may share one instance. Calls happen only on the game thread inside
 *          FReplicationSystem::Tick; any locking belongs to the implementation, never to the system.
 */
class SCENE_EXPORT IReplicationTransport
{
public:

    virtual ~IReplicationTransport () noexcept = default;

    /** @brief Sends one complete frame; a full transport returns CapacityExceeded without partial writes. */
    [[nodiscard]] virtual TExpected<void, EReplicationError> TrySend ( TSpan<const Byte> InBytes ) noexcept = 0;

    /** @brief Receives one complete frame; zero bytes means no frame is available. */
    [[nodiscard]] virtual TExpected<USize, EReplicationError> TryReceive ( TSpan<Byte> OutBytes ) noexcept = 0;
};

} // namespace Lumen
