/**
 * @file RHIError.hpp
 * @brief Defines structured non-throwing RHI failures.
 */
#pragma once

#include <Core/CoreTypes.hpp>
#include <RHIExport.hpp>
#include <String/String.hpp>
#include <String/StringView.hpp>
#include <Templates/Expected.hpp>
#include <Templates/Optional.hpp>

namespace Lumen
{

/** @enum ERHIErrorCode @brief Stable high-level failure codes. */
enum class ERHIErrorCode : UInt16
{
    InvalidArgument,
    InvalidState,
    Unsupported,
    OutOfMemory,
    DeviceLost,
    DeviceClosed,
    Timeout,
    NotFound,
    AlreadyExists,
    ValidationFailed,
    CorruptedData,
    BackendFailure,
    InternalFailure,
};

/** @enum ERHIErrorCondition @brief Subsystem or condition that caused a failure. */
enum class ERHIErrorCondition : UInt16
{
    None,
    Capability,
    Limit,
    Queue,
    Resource,
    Memory,
    Binding,
    Shader,
    Pipeline,
    Command,
    Synchronization,
    Presentation,
    Query,
    Cache,
    Device,
};

/**
 * @struct FRHIError
 * @brief Owned error value propagated by every fallible RHI operation.
 */
struct RHI_EXPORT FRHIError
{
    ERHIErrorCode Code           = ERHIErrorCode::InternalFailure;
    ERHIErrorCondition Condition = ERHIErrorCondition::None;
    FString Message;
    TOptional<Int64> NativeCode;

    /** @brief Creates an error with an owned message and optional native result. */
    FRHIError ( ERHIErrorCode InCode,
                ERHIErrorCondition InCondition,
                FStringView InMessage,
                TOptional<Int64> InNativeCode = {} ) noexcept;

    /** @return A copy with additional operation context prepended to the message. */
    [[nodiscard]] FRHIError WithContext ( FStringView InOperation ) const noexcept;
};

/** @brief Creates an invalid-argument error with a consistent condition. */
[[nodiscard]] RHI_EXPORT FRHIError MakeRHIInvalidArgument ( ERHIErrorCondition InCondition, FStringView InMessage ) noexcept;

} // namespace Lumen
