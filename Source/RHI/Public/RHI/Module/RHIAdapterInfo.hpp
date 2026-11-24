/**
 * @file RHIAdapterInfo.hpp
 * @brief Declares immutable adapter discovery information.
 */
#pragma once

#include <RHI/Types/RHICapabilities.hpp>
#include <RHI/Types/RHILimits.hpp>
#include <RHI/Types/RHIQueueType.hpp>
#include <RHIExport.hpp>
#include <String/String.hpp>
#include <Templates/Vector.hpp>

namespace Lumen
{

namespace RHI
{

    /**
     * @enum ERHIAdapterType
     * @brief Broad adapter class used for selection and diagnostics.
     */
    enum class ERHIAdapterType : UInt8
    {
        Unknown,
        Discrete,
        Integrated,
        Software
    };

    /**
     * @struct FRHIAdapterInfo
     * @brief Capabilities and identity of one physical adapter.
     */
    struct RHI_EXPORT FRHIAdapterInfo
    {
        UInt64 AdapterId = 0ULL;
        FString Name;
        FString BackendName;
        ERHIAdapterType Type = ERHIAdapterType::Unknown;
        FRHICapabilities Capabilities;
        FRHILimits Limits;
        TVector<FRHIQueueFamilyInfo> QueueFamilies;

        /** @return Success when identity, limits, and at least one queue are valid. */
        [[nodiscard]] FRHIExpected Validate () const noexcept;
    };

} // namespace RHI

} // namespace Lumen
