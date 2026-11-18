/**
 * @file RHISemaphoreDesc.hpp
 * @brief Defines binary and timeline semaphore creation.
 */

#pragma once

#include <RHI/Types/RHIError.hpp>
#include <RHIExport.hpp>
#include <String/String.hpp>

namespace Lumen
{

namespace RHI
{

    /**
     * @enum ERHISemaphoreType
     * @brief Signal semantics selected when a semaphore is created.
     */
    enum class ERHISemaphoreType : UInt8
    {
        Binary,
        Timeline
    };

    struct RHI_EXPORT FRHISemaphoreDesc
    {
        ERHISemaphoreType Type = ERHISemaphoreType::Timeline;
        UInt64 InitialValue    = 0ULL;
        FString DebugName;

        /**
         * @brief Validates the semaphore type, initial value and debug name.
         * @return Success, or a structured FRHIError with the relevant condition.
         */
        [[nodiscard]] TExpected<void, FRHIError> Validate () const noexcept;
    };

} // namespace RHI

} // namespace Lumen
