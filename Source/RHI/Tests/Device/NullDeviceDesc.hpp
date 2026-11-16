/**
 * @file NullDeviceDesc.hpp
 * @brief Declares configuration for the deterministic Null device.
 */

#pragma once

#include <Device/NullFaultPlan.hpp>
#include <Memory/SharedPtr.hpp>
#include <Module/RHIAdapterInfo.hpp>

namespace Lumen
{

namespace Tests
{

    /** @return Broad but finite limits used by the Null adapter. */
    [[nodiscard]] FRHILimits MakeDefaultNullLimits () noexcept;

    /**
     *     @struct FNullDeviceDesc
     * @brief Adapter description and shared fault schedule.
     */
    struct FNullDeviceDesc
    {
        FRHIAdapterInfo Adapter;
        TSharedPtr<FNullFaultPlan> FaultPlan;

        /** @brief Creates a valid default Null adapter and an empty fault plan. */
        FNullDeviceDesc () noexcept;
    };

} // namespace Tests

} // namespace Lumen
