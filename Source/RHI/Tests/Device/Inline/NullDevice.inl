/**
 * @file NullDevice.inl
 * @brief Implements common unsupported/fault behavior for Null factories.
 */

#pragma once

#include <Device/NullDevice.hpp>

/**
 * Public
 */

template <typename ObjectType>
Lumen::TExpected<Lumen::TSharedRef<ObjectType>, Lumen::FRHIError>
Lumen::Tests::FNullDevice::Unsupported ( const FStringView InOperation ) noexcept
{
    if ( TOptional<FRHIError> Error = CheckOperation( InOperation ) )
    {
        return TUnexpected<FRHIError>( *Error );
    }
    return TUnexpected<FRHIError>( MakeRHIUnsupported( ERHIErrorCondition::Device, InOperation ) );
}
