/**
 * @file RHIBackendSupport.hpp
 * @brief Declares public helpers for backend capability negotiation.
 */

#pragma once

#include <RHI/Module/RHIAdapterInfo.hpp>
#include <RHI/Module/RHIDeviceDesc.hpp>
#include <RHIExport.hpp>

namespace Lumen
{

namespace RHI
{

    /** @return Success when InDesc can be satisfied by InAdapter without degradation. */
    [[nodiscard]] RHI_EXPORT FRHIExpected CheckRHICapabilityGate ( const FRHIAdapterInfo &InAdapter,
                                                                   const FRHIDeviceDesc &InDesc ) noexcept;

} // namespace RHI

} // namespace Lumen
