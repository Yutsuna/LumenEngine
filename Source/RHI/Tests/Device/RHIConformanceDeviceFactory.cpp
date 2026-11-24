/**
 * @file RHIConformanceDeviceFactory.cpp
 * @brief Implements shared conformance factory helpers.
 */

#include <Device/RHIConformanceDeviceFactory.hpp>
#include <RHI/Device/IRHIDevice.hpp>

Lumen::TExpected<Lumen::TSharedRef<Lumen::RHI::IRHIDevice>, Lumen::RHI::FRHIError>
Lumen::RHI::Tests::IRHIConformanceDeviceFactory::CreateDevice () noexcept
{
    return CreateDevice( GetDefaultValidation() );
}

Lumen::TExpected<Lumen::Bool, Lumen::RHI::FRHIError>
Lumen::RHI::Tests::HasRHIConformanceQueueFamily ( const ERHIQueueType InQueueType ) noexcept
{
    TExpected<TVector<FRHIAdapterInfo>, FRHIError> Adapters = GetRHIConformanceDeviceFactory().GetModule()->EnumerateAdapters();
    if ( not Adapters )
    {
        return TUnexpected<FRHIError>( Adapters.Error() );
    }
    for ( const FRHIAdapterInfo &Adapter : *Adapters )
    {
        for ( const FRHIQueueFamilyInfo &Family : Adapter.QueueFamilies )
        {
            if ( Family.Type == InQueueType and Family.Count > 0U )
            {
                return true;
            }
        }
    }
    return false;
}

Lumen::Bool Lumen::RHI::Tests::HasRHIConformanceError ( const FRHIError &InError,
                                                        const ERHIErrorCode InExpectedCode,
                                                        const ERHIErrorCondition InExpectedCondition ) noexcept
{
    return InError.Condition == InExpectedCondition and
           ( InError.Code == InExpectedCode or InError.Code == ERHIErrorCode::ValidationFailed );
}
