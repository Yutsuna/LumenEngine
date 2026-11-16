/**
 * @file RHIDeviceStatus.hpp
 * @brief Defines device lifetime and loss states.
 */
#pragma once

#include <Core/CoreTypes.hpp>

namespace Lumen
{

/** @enum ERHIDeviceStatus @brief Operational state shared by device and child objects. */
enum class ERHIDeviceStatus : UInt8
{
    Ready,
    Lost,
    Closed
};

/** @return True when new GPU work may be created in this state. */
[[nodiscard]] constexpr Bool IsRHIDeviceUsable ( const ERHIDeviceStatus InStatus ) noexcept
{
    return InStatus == ERHIDeviceStatus::Ready;
}

} // namespace Lumen
