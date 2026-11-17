/**
 * @file RHISemaphoreDesc.hpp
 * @brief Defines binary and timeline semaphore creation.
 */

#pragma once
#include <String/String.hpp>
#include <Types/RHIError.hpp>
namespace Lumen
{
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
} // namespace Lumen
