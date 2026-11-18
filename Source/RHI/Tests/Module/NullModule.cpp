/**
 * @file NullModule.cpp
 * @brief Implements the Null RHI module.
 */

#include <Device/NullDevice.hpp>
#include <Device/RHICapabilityGate.hpp>
#include <Module/NullModule.hpp>

/**
 * Public
 */

Lumen::Tests::FNullModule::FNullModule ( FNullDeviceDesc InDesc ) noexcept : Desc( std::move( InDesc ) )
{
    /* */
}

Lumen::FRHIApiVersion Lumen::Tests::FNullModule::GetApiVersion () const noexcept
{
    return {};
}

Lumen::TExpected<Lumen::TVector<Lumen::FRHIAdapterInfo>, Lumen::FRHIError>
Lumen::Tests::FNullModule::EnumerateAdapters () const noexcept
{
    return TVector<FRHIAdapterInfo>{ Desc.Adapter };
}

Lumen::TExpected<Lumen::TSharedRef<Lumen::IRHIDevice>, Lumen::FRHIError>
Lumen::Tests::FNullModule::CreateDevice ( const FRHIDeviceDesc &InDesc ) noexcept
{
    return CheckRHICapabilityGate( Desc.Adapter, InDesc )
        .OrElse(
            [] ( const FRHIError &InError ) noexcept -> TExpected<void, FRHIError>
    {
        return TUnexpected<FRHIError>( InError );
    } )
        .AndThen( [this] () noexcept -> TExpected<TSharedRef<IRHIDevice>, FRHIError>
    {
        return MakeShared<FNullDevice>( Desc );
    } );
}
