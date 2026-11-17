/**
 * @file RHIError.inl
 * @brief Inline implementation of RHIError functions
 */

#pragma once

#include <Types/RHIError.hpp>

/**
 * Make
 */

Lumen::FRHIError Lumen::MakeRHIInvalidArgument ( const ERHIErrorCondition InCondition, const FStringView InMessage ) noexcept
{
    return { ERHIErrorCode::InvalidArgument, InCondition, InMessage };
}

Lumen::FRHIError Lumen::MakeRHIInvalidCapability ( const FStringView InMessage ) noexcept
{
    return MakeRHIInvalidArgument( ERHIErrorCondition::Capability, InMessage );
}

Lumen::FRHIError Lumen::MakeRHIInvalidLimit ( const FStringView InMessage ) noexcept
{
    return MakeRHIInvalidArgument( ERHIErrorCondition::Limit, InMessage );
}

Lumen::FRHIError Lumen::MakeRHIInvalidQueue ( const FStringView InMessage ) noexcept
{
    return MakeRHIInvalidArgument( ERHIErrorCondition::Queue, InMessage );
}

Lumen::FRHIError Lumen::MakeRHIInvalidResource ( const FStringView InMessage ) noexcept
{
    return MakeRHIInvalidArgument( ERHIErrorCondition::Resource, InMessage );
}

Lumen::FRHIError Lumen::MakeRHIInvalidMemory ( const FStringView InMessage ) noexcept
{
    return MakeRHIInvalidArgument( ERHIErrorCondition::Memory, InMessage );
}

Lumen::FRHIError Lumen::MakeRHIInvalidBinding ( const FStringView InMessage ) noexcept
{
    return MakeRHIInvalidArgument( ERHIErrorCondition::Binding, InMessage );
}

Lumen::FRHIError Lumen::MakeRHIInvalidShader ( const FStringView InMessage ) noexcept
{
    return MakeRHIInvalidArgument( ERHIErrorCondition::Shader, InMessage );
}

Lumen::FRHIError Lumen::MakeRHIInvalidPipeline ( const FStringView InMessage ) noexcept
{
    return MakeRHIInvalidArgument( ERHIErrorCondition::Pipeline, InMessage );
}

Lumen::FRHIError Lumen::MakeRHIInvalidCommand ( const FStringView InMessage ) noexcept
{
    return MakeRHIInvalidArgument( ERHIErrorCondition::Command, InMessage );
}

Lumen::FRHIError Lumen::MakeRHIInvalidSynchronization ( const FStringView InMessage ) noexcept
{
    return MakeRHIInvalidArgument( ERHIErrorCondition::Synchronization, InMessage );
}

Lumen::FRHIError Lumen::MakeRHIInvalidPresentation ( const FStringView InMessage ) noexcept
{
    return MakeRHIInvalidArgument( ERHIErrorCondition::Presentation, InMessage );
}

Lumen::FRHIError Lumen::MakeRHIInvalidQuery ( const FStringView InMessage ) noexcept
{
    return MakeRHIInvalidArgument( ERHIErrorCondition::Query, InMessage );
}

Lumen::FRHIError Lumen::MakeRHIInvalidCache ( const FStringView InMessage ) noexcept
{
    return MakeRHIInvalidArgument( ERHIErrorCondition::Cache, InMessage );
}

Lumen::FRHIError Lumen::MakeRHIInvalidDevice ( const FStringView InMessage ) noexcept
{
    return MakeRHIInvalidArgument( ERHIErrorCondition::Device, InMessage );
}

/**
 * OutOfMemory
 */

LUMEN_FORCE_INLINE Lumen::FRHIError Lumen::MakeRHIOutOfMemory ( const Lumen::ERHIErrorCondition Condition,
                                                                const Lumen::FStringView Message ) noexcept
{
    return FRHIError{ ERHIErrorCode::OutOfMemory, Condition, Message };
}

/**
 * Unsupported
 */

LUMEN_FORCE_INLINE Lumen::FRHIError Lumen::MakeRHIUnsupported ( const Lumen::ERHIErrorCondition Condition,
                                                                const Lumen::FStringView InMessage ) noexcept
{
    return FRHIError{ ERHIErrorCode::Unsupported, Condition, InMessage };
}

/**
 * Invalid State
 */

LUMEN_FORCE_INLINE Lumen::FRHIError Lumen::MakeRHIInvalidState ( const Lumen::ERHIErrorCondition Condition,
                                                                 const Lumen::FStringView InMessage ) noexcept
{
    return FRHIError{ ERHIErrorCode::InvalidState, ERHIErrorCondition::None, InMessage };
}
