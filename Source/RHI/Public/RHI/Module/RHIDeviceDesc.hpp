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

    class IRHIDebugSink;

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
        /** NOTE: Non-owning; a supplied sink must outlive device and native callback teardown. */
        IRHIDebugSink *DebugSink = nullptr;

        /** @return Success when queue requests are nonzero and not duplicated. */
        [[nodiscard]] FRHIExpected Validate () const noexcept;
    };

} // namespace RHI

} // namespace Lumen
