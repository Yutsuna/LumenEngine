/**
 * @file RHIError.hpp
 * @brief Defines structured non-throwing RHI failures.
 */
#pragma once

#include <RHIExport.hpp>

#include <Core/CoreTypes.hpp>

#include <String/String.hpp>
#include <String/StringView.hpp>

#include <Templates/Expected.hpp>
#include <Templates/Optional.hpp>

namespace Lumen
{

/**
 * @enum ERHIErrorCode
 * @brief Stable high-level failure codes.
 */
enum class ERHIErrorCode : UInt16
{
    InvalidArgument,
    InvalidState,
    Unsupported,
    OutOfMemory,
    DeviceLost,
    DeviceClosed,
    OutOfDate,
    Timeout,
    NotFound,
    AlreadyExists,
    ValidationFailed,
    CorruptedData,
    BackendFailure,
    InternalFailure,
};

/**
 * @enum ERHIErrorCondition
 * @brief Subsystem or condition that caused a failure.
 */
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
[[nodiscard]] LUMEN_FORCE_INLINE RHI_EXPORT FRHIError MakeRHIInvalidArgument ( ERHIErrorCondition InCondition,
                                                                               FStringView InMessage ) noexcept;

/** @brief Creates an invalid-capability error with a consistent condition. */
[[nodiscard]] LUMEN_FORCE_INLINE RHI_EXPORT FRHIError MakeRHIInvalidCapability ( FStringView InMessage ) noexcept;

/** @brief Creates an invalid-limit error with a consistent condition. */
[[nodiscard]] LUMEN_FORCE_INLINE RHI_EXPORT FRHIError MakeRHIInvalidLimit ( FStringView InMessage ) noexcept;

/** @brief Creates an invalid-queue error with a consistent condition. */
[[nodiscard]] LUMEN_FORCE_INLINE RHI_EXPORT FRHIError MakeRHIInvalidQueue ( FStringView InMessage ) noexcept;

/** @brief Creates an invalid-resource error with a consistent condition. */
[[nodiscard]] LUMEN_FORCE_INLINE RHI_EXPORT FRHIError MakeRHIInvalidResource ( FStringView InMessage ) noexcept;

/** @brief Creates an invalid-memory error with a consistent condition. */
[[nodiscard]] LUMEN_FORCE_INLINE RHI_EXPORT FRHIError MakeRHIInvalidMemory ( FStringView InMessage ) noexcept;

/** @brief Creates an invalid-binding error with a consistent condition. */
[[nodiscard]] LUMEN_FORCE_INLINE RHI_EXPORT FRHIError MakeRHIInvalidBinding ( FStringView InMessage ) noexcept;

/** @brief Creates an invalid-shader error with a consistent condition. */
[[nodiscard]] LUMEN_FORCE_INLINE RHI_EXPORT FRHIError MakeRHIInvalidShader ( FStringView InMessage ) noexcept;

/** @brief Creates an invalid-pipeline error with a consistent condition. */
[[nodiscard]] LUMEN_FORCE_INLINE RHI_EXPORT FRHIError MakeRHIInvalidPipeline ( FStringView InMessage ) noexcept;

/** @brief Creates an invalid-command error with a consistent condition. */
[[nodiscard]] LUMEN_FORCE_INLINE RHI_EXPORT FRHIError MakeRHIInvalidCommand ( FStringView InMessage ) noexcept;

/** @brief Creates an invalid-synchronization error with a consistent condition. */
[[nodiscard]] LUMEN_FORCE_INLINE RHI_EXPORT FRHIError MakeRHIInvalidSynchronization ( FStringView InMessage ) noexcept;

/** @brief Creates an invalid-Presentation error with a consistent condition. */
[[nodiscard]] LUMEN_FORCE_INLINE RHI_EXPORT FRHIError MakeRHIInvalidPresentation ( FStringView InMessage ) noexcept;

/** @brief Creates an invalid-Query error with a consistent condition. */
[[nodiscard]] LUMEN_FORCE_INLINE RHI_EXPORT FRHIError MakeRHIInvalidQuery ( FStringView InMessage ) noexcept;

/** @brief Creates an invalid-Cache error with a consistent condition. */
[[nodiscard]] LUMEN_FORCE_INLINE RHI_EXPORT FRHIError MakeRHIInvalidCache ( FStringView InMessage ) noexcept;

/** @brief Creates an invalid-Device error with a consistent condition. */
[[nodiscard]] LUMEN_FORCE_INLINE RHI_EXPORT FRHIError MakeRHIInvalidDevice ( FStringView InMessage ) noexcept;

/** @brief Creates an out of memory error. */
[[nodiscard]] LUMEN_FORCE_INLINE RHI_EXPORT FRHIError MakeRHIOutOfMemory ( ERHIErrorCondition Condition,
                                                                           FStringView Message ) noexcept;

/** @brief Creates an unsupported error. */
[[nodiscard]] LUMEN_FORCE_INLINE RHI_EXPORT FRHIError MakeRHIUnsupported ( ERHIErrorCondition Condition,
                                                                           FStringView InMessage ) noexcept;

/** @brief Creates an invalid state error */
[[nodiscard]] LUMEN_FORCE_INLINE RHI_EXPORT FRHIError MakeRHIInvalidState ( ERHIErrorCondition Condition,
                                                                            FStringView InMessage ) noexcept;

/** @brief Creates a corrupted error */
[[nodiscard]] LUMEN_FORCE_INLINE RHI_EXPORT FRHIError MakeRHICorrupted ( ERHIErrorCondition Condition,
                                                                         FStringView InMessage ) noexcept;

/** @brief Creates a timeout error */
[[nodiscard]] LUMEN_FORCE_INLINE RHI_EXPORT FRHIError MakeRHITimeout ( const Lumen::ERHIErrorCondition Condition,
                                                                       const Lumen::FStringView InMessage ) noexcept;

/** @brief Creates a device lost error */
[[nodiscard]] LUMEN_FORCE_INLINE RHI_EXPORT FRHIError MakeRHIDeviceLost ( const Lumen::ERHIErrorCondition Condition,
                                                                          const Lumen::FStringView InMessage ) noexcept;

/** @brief Creates a validation failed error */
[[nodiscard]] LUMEN_FORCE_INLINE RHI_EXPORT FRHIError MakeRHIValidationFailed ( const Lumen::ERHIErrorCondition Condition,
                                                                                const Lumen::FStringView InMessage ) noexcept;
/** @brief Creates a Not found error */
[[nodiscard]] LUMEN_FORCE_INLINE RHI_EXPORT FRHIError MakeRHINotFound ( const Lumen::ERHIErrorCondition Condition,
                                                                         const Lumen::FStringView InMessage ) noexcept;

} // namespace Lumen

#include "Inline/RHIError.inl"
