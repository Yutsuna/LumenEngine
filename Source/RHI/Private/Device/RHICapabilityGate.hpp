/**
 * @file RHICapabilityGate.hpp
 * @brief Declares the pure device requirement gate.
 */

#pragma once

#include <RHI/Module/RHIAdapterInfo.hpp>
#include <RHI/Module/RHIDeviceDesc.hpp>

namespace Lumen
{

namespace RHI
{

    /** @return Success when InDesc can be satisfied by InAdapter without degradation. */
    [[nodiscard]] TExpected<void, FRHIError> CheckRHICapabilityGate ( const FRHIAdapterInfo &InAdapter,
                                                                      const FRHIDeviceDesc &InDesc ) noexcept;

} // namespace RHI

} // namespace Lumen

#include "Inline/RHICapabilityGate.inl"
