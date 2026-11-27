/**
 * @file NullModule.cpp
 * @brief Implements the Null RHI module.
 */

#include <Device/NullDevice.hpp>
#include <Module/NullModule.hpp>
#include <RHI/Device/RHIBackendSupport.hpp>

/**
 * Public
 */

Lumen::RHI::Tests::FNullModule::FNullModule ( FNullDeviceDesc InDesc ) noexcept : Desc( std::move( InDesc ) )
{
    /* */
}

Lumen::RHI::FRHIApiVersion Lumen::RHI::Tests::FNullModule::GetApiVersion () const noexcept
{
    return { .Major = 1U, .Minor = 1U, .Patch = 0U };
}

Lumen::TExpected<Lumen::TVector<Lumen::RHI::FRHIAdapterInfo>, Lumen::RHI::FRHIError>
Lumen::RHI::Tests::FNullModule::EnumerateAdapters () const noexcept
{
    return TVector<FRHIAdapterInfo>{ Desc.Adapter };
}

Lumen::TExpected<Lumen::TSharedRef<Lumen::RHI::IRHIDevice>, Lumen::RHI::FRHIError>
Lumen::RHI::Tests::FNullModule::CreateDevice ( const FRHIDeviceDesc &InDesc ) noexcept
{
    return CheckRHICapabilityGate( Desc.Adapter, InDesc )
        .OrElse(
            [] ( const FRHIError &InError ) noexcept -> FRHIExpected
    {
        return TUnexpected<FRHIError>( InError );
    } )
        .AndThen( [this] () noexcept -> TExpected<TSharedRef<IRHIDevice>, FRHIError>
    {
        return MakeShared<FNullDevice>( Desc );
    } );
}
