/**
 * @file RHICapabilityGate.hpp
 * @brief Declares the pure device requirement gate.
 */

#pragma once

#include <Module/RHIAdapterInfo.hpp>
#include <Module/RHIDeviceDesc.hpp>

namespace Lumen
{

/** @return Success when InDesc can be satisfied by InAdapter without degradation. */
[[nodiscard]] TExpected<void, FRHIError> CheckRHICapabilityGate ( const FRHIAdapterInfo &InAdapter,
                                                                  const FRHIDeviceDesc &InDesc ) noexcept;

} // namespace Lumen

#include "Inline/RHICapabilityGate.inl"
