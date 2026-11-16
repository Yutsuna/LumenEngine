/**
 * @file NullConformanceDeviceFactory.cpp
 * @brief Implements Null conformance device creation.
 */

#include "Device/NullConformanceDeviceFactory.hpp"
#include "Module/NullModule.hpp"

/**
 * Public
 */

Lumen::Tests::FNullConformanceDeviceFactory::FNullConformanceDeviceFactory ( FNullDeviceDesc InDesc ) noexcept
    : Desc( std::move( InDesc ) )
{
    /* */
}

Lumen::TSharedRef<Lumen::IRHIModule> Lumen::Tests::FNullConformanceDeviceFactory::GetModule () const noexcept
{
    return MakeShared<FNullModule>( Desc );
}

Lumen::TExpected<Lumen::TSharedRef<Lumen::IRHIDevice>, Lumen::FRHIError>
Lumen::Tests::FNullConformanceDeviceFactory::CreateDevice ( const Bool bEnableValidation ) noexcept
{
    if ( bEnableValidation )
    {
        return TUnexpected<FRHIError>(
            FRHIError( ERHIErrorCode::Unsupported, ERHIErrorCondition::Device, "Validation is not wired yet" ) );
    }
    return GetModule()->CreateDevice( FRHIDeviceDesc{} );
}

Lumen::Tests::IRHIConformanceDeviceFactory &Lumen::Tests::GetRHIConformanceDeviceFactory () noexcept
{
    static FNullConformanceDeviceFactory Factory;
    return Factory;
}
