/**
 * @file RHIValidationDesc.cpp
 * @brief Implements validation descriptor checks and coverage selection.
 */
#include <Validation/RHIValidationDesc.hpp>

Lumen::TExpected<void, Lumen::FRHIError> Lumen::FRHIValidationDesc::Validate () const noexcept
{
    if ( Level >= ERHIValidationLevel::Count )
    {
        return TUnexpected<FRHIError>( MakeRHIInvalidDevice( "Unknown validation level" ) );
    }
    constexpr UInt32 ValidFlags = static_cast<UInt32>( ERHIValidationFlags::All );
    if ( ( static_cast<UInt32>( Flags ) & ~ValidFlags ) != 0U )
    {
        return TUnexpected<FRHIError>( MakeRHIInvalidDevice( "Unknown validation flags" ) );
    }
    if ( MinimumSeverity >= ERHIDebugSeverity::Count )
    {
        return TUnexpected<FRHIError>( MakeRHIInvalidDevice( "Unknown validation severity" ) );
    }
    return {};
}

Lumen::Bool Lumen::FRHIValidationDesc::IsEnabled ( const ERHIValidationFlags InDomain ) const noexcept
{
    if ( Level == ERHIValidationLevel::Disabled or not HasAnyFlags( Flags, InDomain ) )
    {
        return false;
    }
    if ( Level == ERHIValidationLevel::Full )
    {
        return true;
    }
    constexpr ERHIValidationFlags BasicFlags = ERHIValidationFlags::ObjectLifetime | ERHIValidationFlags::ResourceStates |
                                               ERHIValidationFlags::Commands | ERHIValidationFlags::Queues;
    return HasAnyFlags( BasicFlags, InDomain );
}
