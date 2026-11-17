/**
 * @file RHIValidation.cpp
 * @brief Implements validation decoration and recursive backend unwrapping.
 */
#include "RHIValidationDevice.hpp"
#include <Validation/RHIValidation.hpp>

Lumen::TExpected<Lumen::TSharedRef<Lumen::IRHIDevice>, Lumen::FRHIError>
Lumen::WrapRHIDevice ( const TSharedRef<IRHIDevice> &InDevice, const FRHIValidationDesc &InDesc ) noexcept
{
    return InDesc.Validate().AndThen( [&InDevice, &InDesc] () noexcept -> TExpected<TSharedRef<IRHIDevice>, FRHIError>
    {
        if ( InDesc.Level == ERHIValidationLevel::Disabled )
        {
            return InDevice;
        }
        const TSharedRef<FRHIValidationReporter> Reporter = MakeShared<FRHIValidationReporter>( InDesc );
        const TSharedPtr<FRHIValidationState> State       = MakeShared<FRHIValidationState>( Reporter );
        return MakeShared<FRHIValidationDevice>( InDevice, State );
    } );
}

Lumen::IRHIObject *Lumen::UnwrapRHIObject ( IRHIObject *InObject ) noexcept
{
    IRHIObject *Current = InObject;
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

const Lumen::IRHIObject *Lumen::UnwrapRHIObject ( const IRHIObject *InObject ) noexcept
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

Lumen::TSharedRef<Lumen::IRHIObject> Lumen::UnwrapRHISharedObject ( const TSharedRef<IRHIObject> &InObject ) noexcept
{
    TSharedRef<IRHIObject> Current = InObject;
    while ( const auto *Validation = dynamic_cast<const IRHIValidationObject *>( Current.Get() ) )
    {
        Current = Validation->GetValidationWrappedShared();
    }
    return Current;
}

Lumen::TSharedPtr<Lumen::IRHIObject> Lumen::UnwrapRHISharedObject ( const TSharedPtr<IRHIObject> &InObject ) noexcept
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
