/**
 * @file RHIValidation.cpp
 * @brief Implements validation decoration and recursive backend unwrapping.
 */

#include "RHIValidationDevice.hpp"
#include <RHI/Validation/RHIValidation.hpp>

/**
 * Public
 */

Lumen::TExpected<Lumen::TSharedRef<Lumen::RHI::IRHIDevice>, Lumen::RHI::FRHIError>
Lumen::RHI::WrapRHIDevice ( const TSharedRef<IRHIDevice> &InDevice, const FRHIValidationDesc &InDesc ) noexcept
{
    LUMEN_TRY_EXPECT_ERROR( InDesc.Validate() );

    if ( InDesc.Level == ERHIValidationLevel::Disabled )
    {
        return InDevice;
    }
    return MakeShared<FRHIValidationDevice>( InDevice, FRHIValidationState::Create( InDesc ) );
}

Lumen::RHI::IRHIObject *Lumen::RHI::UnwrapRHIObject ( IRHIObject *InObject ) noexcept
{
    IRHIObject *Current = InObject; /** NOLINT(misc-const-correctness) */
    while ( Current != nullptr )
    {
        const auto *Validation = dynamic_cast<const IRHIValidationObject *>( Current );
        if ( Validation == nullptr )
        {
            break;
        }
        Current = &Validation->GetValidationWrappedObject();
    }
    return Current;
}

const Lumen::RHI::IRHIObject *Lumen::RHI::UnwrapRHIObject ( const IRHIObject *InObject ) noexcept
{
    const IRHIObject *Current = InObject;
    while ( Current != nullptr )
    {
        const auto *Validation = dynamic_cast<const IRHIValidationObject *>( Current );
        if ( Validation == nullptr )
        {
            break;
        }
        Current = &Validation->GetValidationWrappedObject();
    }
    return Current;
}

Lumen::TSharedRef<Lumen::RHI::IRHIObject> Lumen::RHI::UnwrapRHISharedObject ( const TSharedRef<IRHIObject> &InObject ) noexcept
{
    TSharedRef<IRHIObject> Current = InObject;
    while ( const auto *Validation = dynamic_cast<const IRHIValidationObject *>( Current.Get() ) )
    {
        Current = Validation->GetValidationWrappedShared();
    }
    return Current;
}

Lumen::TSharedPtr<Lumen::RHI::IRHIObject> Lumen::RHI::UnwrapRHISharedObject ( const TSharedPtr<IRHIObject> &InObject ) noexcept
{
    TSharedPtr<IRHIObject> Current = InObject;
    while ( Current )
    {
        const auto *Validation = dynamic_cast<const IRHIValidationObject *>( Current.Get() );
        if ( Validation == nullptr )
        {
            break;
        }
        Current = Validation->GetValidationWrappedShared();
    }
    return Current;
}
