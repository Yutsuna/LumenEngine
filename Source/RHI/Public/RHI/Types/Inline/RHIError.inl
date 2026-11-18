/**
 * @file RHIError.inl
 * @brief Inline implementation of RHIError functions
 */

#pragma once

#include <RHI/Types/RHIError.hpp>

/**
 * Make
 */

Lumen::RHI::FRHIError Lumen::RHI::MakeRHIInvalidArgument ( const ERHIErrorCondition InCondition,
                                                           const FStringView InMessage ) noexcept
{
    return { ERHIErrorCode::InvalidArgument, InCondition, InMessage };
}

Lumen::RHI::FRHIError Lumen::RHI::MakeRHIInvalidCapability ( const FStringView InMessage ) noexcept
{
    return MakeRHIInvalidArgument( ERHIErrorCondition::Capability, InMessage );
}

Lumen::RHI::FRHIError Lumen::RHI::MakeRHIInvalidLimit ( const FStringView InMessage ) noexcept
{
    return MakeRHIInvalidArgument( ERHIErrorCondition::Limit, InMessage );
}

Lumen::RHI::FRHIError Lumen::RHI::MakeRHIInvalidQueue ( const FStringView InMessage ) noexcept
{
    return MakeRHIInvalidArgument( ERHIErrorCondition::Queue, InMessage );
}

Lumen::RHI::FRHIError Lumen::RHI::MakeRHIInvalidResource ( const FStringView InMessage ) noexcept
{
    return MakeRHIInvalidArgument( ERHIErrorCondition::Resource, InMessage );
}

Lumen::RHI::FRHIError Lumen::RHI::MakeRHIInvalidMemory ( const FStringView InMessage ) noexcept
{
    return MakeRHIInvalidArgument( ERHIErrorCondition::Memory, InMessage );
}

Lumen::RHI::FRHIError Lumen::RHI::MakeRHIInvalidBinding ( const FStringView InMessage ) noexcept
{
    return MakeRHIInvalidArgument( ERHIErrorCondition::Binding, InMessage );
}

Lumen::RHI::FRHIError Lumen::RHI::MakeRHIInvalidShader ( const FStringView InMessage ) noexcept
{
    return MakeRHIInvalidArgument( ERHIErrorCondition::Shader, InMessage );
}

Lumen::RHI::FRHIError Lumen::RHI::MakeRHIInvalidPipeline ( const FStringView InMessage ) noexcept
{
    return MakeRHIInvalidArgument( ERHIErrorCondition::Pipeline, InMessage );
}

Lumen::RHI::FRHIError Lumen::RHI::MakeRHIInvalidCommand ( const FStringView InMessage ) noexcept
{
    return MakeRHIInvalidArgument( ERHIErrorCondition::Command, InMessage );
}

Lumen::RHI::FRHIError Lumen::RHI::MakeRHIInvalidSynchronization ( const FStringView InMessage ) noexcept
{
    return MakeRHIInvalidArgument( ERHIErrorCondition::Synchronization, InMessage );
}

Lumen::RHI::FRHIError Lumen::RHI::MakeRHIInvalidPresentation ( const FStringView InMessage ) noexcept
{
    return MakeRHIInvalidArgument( ERHIErrorCondition::Presentation, InMessage );
}

Lumen::RHI::FRHIError Lumen::RHI::MakeRHIInvalidQuery ( const FStringView InMessage ) noexcept
{
    return MakeRHIInvalidArgument( ERHIErrorCondition::Query, InMessage );
}

Lumen::RHI::FRHIError Lumen::RHI::MakeRHIInvalidCache ( const FStringView InMessage ) noexcept
{
    return MakeRHIInvalidArgument( ERHIErrorCondition::Cache, InMessage );
}

Lumen::RHI::FRHIError Lumen::RHI::MakeRHIInvalidDevice ( const FStringView InMessage ) noexcept
{
    return MakeRHIInvalidArgument( ERHIErrorCondition::Device, InMessage );
}

/**
 * OutOfMemory
 */

Lumen::RHI::FRHIError Lumen::RHI::MakeRHIOutOfMemory ( const Lumen::RHI::ERHIErrorCondition Condition,
                                                       const Lumen::FStringView Message ) noexcept
{
    return FRHIError{ ERHIErrorCode::OutOfMemory, Condition, Message };
}

/**
 * Unsupported
 */

Lumen::RHI::FRHIError Lumen::RHI::MakeRHIUnsupported ( const Lumen::RHI::ERHIErrorCondition Condition,
                                                       const Lumen::FStringView InMessage ) noexcept
{
    return FRHIError{ ERHIErrorCode::Unsupported, Condition, InMessage };
}

/**
 * Invalid State
 */

Lumen::RHI::FRHIError Lumen::RHI::MakeRHIInvalidState ( const Lumen::RHI::ERHIErrorCondition Condition,
                                                        const Lumen::FStringView InMessage ) noexcept
{
    return FRHIError{ ERHIErrorCode::InvalidState, Condition, InMessage };
}

/**
 * Corrupted
 */

Lumen::RHI::FRHIError Lumen::RHI::MakeRHICorrupted ( const Lumen::RHI::ERHIErrorCondition Condition,
                                                     const Lumen::FStringView InMessage ) noexcept
{
    return FRHIError{ ERHIErrorCode::CorruptedData, Condition, InMessage };
}

/**
 * Timed Out
 */

Lumen::RHI::FRHIError Lumen::RHI::MakeRHITimeout ( const Lumen::RHI::ERHIErrorCondition Condition,
                                                   const Lumen::FStringView InMessage ) noexcept
{
    return FRHIError{ ERHIErrorCode::Timeout, Condition, InMessage };
}

/**
 * Device Lost
 */

Lumen::RHI::FRHIError Lumen::RHI::MakeRHIDeviceLost ( const Lumen::RHI::ERHIErrorCondition Condition,
                                                      const Lumen::FStringView InMessage ) noexcept
{
    return FRHIError{ ERHIErrorCode::DeviceLost, Condition, InMessage };
}

/**
 * Validation Failed
 */

Lumen::RHI::FRHIError Lumen::RHI::MakeRHIValidationFailed ( const Lumen::RHI::ERHIErrorCondition Condition,
                                                            const Lumen::FStringView InMessage ) noexcept
{
    return FRHIError{ ERHIErrorCode::ValidationFailed, Condition, InMessage };
}

/**
 * Not Found
 */

Lumen::RHI::FRHIError Lumen::RHI::MakeRHINotFound ( const Lumen::RHI::ERHIErrorCondition Condition,
                                                    const Lumen::FStringView InMessage ) noexcept
{
    return FRHIError{ ERHIErrorCode::NotFound, Condition, InMessage };
}

/**
 * Out Of Date
 */

Lumen::RHI::FRHIError Lumen::RHI::MakeRHIOutOfDate ( const Lumen::RHI::ERHIErrorCondition Condition,
                                                     const Lumen::FStringView InMessage ) noexcept
{
    return FRHIError{ ERHIErrorCode::OutOfDate, Condition, InMessage };
}

/**
 * Backend Failure
 */

Lumen::RHI::FRHIError Lumen::RHI::MakeRHIBackendFailure ( const Lumen::RHI::ERHIErrorCondition Condition,
                                                          const Lumen::FStringView InMessage ) noexcept
{
    return FRHIError{ ERHIErrorCode::BackendFailure, Condition, InMessage };
}
