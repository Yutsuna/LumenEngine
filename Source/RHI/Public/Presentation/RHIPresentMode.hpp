/**
 * @file RHIPresentMode.hpp
 * @brief Defines portable swapchain presentation scheduling modes.
 */
#pragma once

#include <Core/CoreTypes.hpp>

namespace Lumen
{

/**
 * @enum ERHIPresentMode
 * @brief Presentation latency and queueing policy.
 */
enum class ERHIPresentMode : UInt8
{
    Fifo,
    Immediate,
    Mailbox,
    Count
};

} // namespace Lumen
