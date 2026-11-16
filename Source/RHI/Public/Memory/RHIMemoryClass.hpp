/**
 * @file RHIMemoryClass.hpp
 * @brief Defines portable memory classes.
 */
#pragma once
#include <Core/CoreTypes.hpp>
namespace Lumen
{
/**
 * @enum ERHIMemoryClass
 * @brief Device, upload, and readback memory categories.
 */
enum class ERHIMemoryClass : UInt8
{
    DeviceLocal,
    Upload,
    Readback
};
/** @return True when CPU mapping is defined for this class. */
[[nodiscard]] constexpr Bool IsRHICpuAccessible ( const ERHIMemoryClass InClass ) noexcept
{
    return InClass == ERHIMemoryClass::Upload or InClass == ERHIMemoryClass::Readback;
}
} // namespace Lumen
