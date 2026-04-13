/**
 * @file ActorMailbox.hpp
 * @brief Actor mailbox definition for parallel processing in Lumen Engine.
 */

#pragma once

#include "Container/Optional.hpp"
#include "Thread/Mutex.hpp"

namespace LumenEngine
{

struct FMessage;

/**
 * @class FMailBox
 * @brief MPSC (multi-producer, single-consumer) mailbox for actors in Lumen Engine.
 */
class FMailBox final
{
public:

    FMailBox() noexcept = default;
    ~FMailBox() noexcept = default;

public:

    void Push(FMessage InMessage) noexcept;
    [[nodiscard]] TOptional<FMessage> Pop() noexcept;
    [[nodiscard]]bool IsEmpty() const noexcept;

private:

    FMutex Mutex = {};
    TQueue<FMessage> Queue = {};

}

} // namespace LumenEngine