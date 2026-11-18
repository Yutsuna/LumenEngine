/**
 * @file RHIDeviceDesc.hpp
 * @brief Declares device creation requirements.
 */
#pragma once

#include <RHI/Types/RHICapabilities.hpp>
#include <RHI/Types/RHIQueueType.hpp>
#include <RHIExport.hpp>
#include <Templates/Optional.hpp>
#include <Templates/Vector.hpp>

namespace Lumen
{

namespace RHI
{

    /**
     * @struct FRHIQueueRequest
     * @brief Requests InCount queues of one class.
     */
    struct FRHIQueueRequest
    {
        ERHIQueueType Type = ERHIQueueType::Graphics;
        UInt32 Count       = 1U;
    };

    /**
     * @struct FRHIDeviceDesc
     * @brief Adapter selection and required device features.
     */
    struct RHI_EXPORT FRHIDeviceDesc
    {
        TOptional<UInt64> AdapterId;
        FRHICapabilities RequiredCapabilities;
        TVector<FRHIQueueRequest> Queues = { FRHIQueueRequest{} };
        Bool bEnableDebug                = false;

        /** @return Success when queue requests are nonzero and not duplicated. */
        [[nodiscard]] TExpected<void, FRHIError> Validate () const noexcept;
    };

} // namespace RHI

} // namespace Lumen
