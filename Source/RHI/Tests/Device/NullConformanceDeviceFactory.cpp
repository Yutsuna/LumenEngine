/**
 * @file NullConformanceDeviceFactory.cpp
 * @brief Implements Null conformance device creation.
 */

#include "Device/NullConformanceDeviceFactory.hpp"
#include "Module/NullModule.hpp"
#include <Device/NullDevice.hpp>
#include <Presentation/NullPresentation.hpp>
#include <RHI/Presentation/RHISwapchainDesc.hpp>
#include <RHI/Shaders/RHIShaderDesc.hpp>
#include <RHI/Validation/RHIValidation.hpp>
#include <Templates/Optional.hpp>

/**
 * Public
 */

Lumen::RHI::Tests::FNullConformanceDeviceFactory::FNullConformanceDeviceFactory ( FNullDeviceDesc InDesc ) noexcept
    : Desc( std::move( InDesc ) )
{
    /* */
}

Lumen::TSharedRef<Lumen::RHI::IRHIModule> Lumen::RHI::Tests::FNullConformanceDeviceFactory::GetModule () const noexcept
{
    return MakeShared<FNullModule>( Desc );
}

Lumen::TExpected<Lumen::TSharedRef<Lumen::RHI::IRHIDevice>, Lumen::RHI::FRHIError>
Lumen::RHI::Tests::FNullConformanceDeviceFactory::CreateDevice ( const Bool bEnableValidation ) noexcept
{
    TExpected<TSharedRef<IRHIDevice>, FRHIError> Device = GetModule()->CreateDevice( FRHIDeviceDesc{} );
    if ( not Device or not bEnableValidation )
    {
        return Device;
    }
    return WrapRHIDevice( *Device );
}

void Lumen::RHI::Tests::FNullConformanceDeviceFactory::SetDefaultValidation ( const Bool bEnableValidation ) noexcept
{
    bDefaultValidation = bEnableValidation;
}

Lumen::Bool Lumen::RHI::Tests::FNullConformanceDeviceFactory::GetDefaultValidation () const noexcept
{
    return bDefaultValidation;
}

Lumen::TExpected<Lumen::TSharedRef<Lumen::RHI::IRHIShader>, Lumen::RHI::FRHIError>
Lumen::RHI::Tests::FNullConformanceDeviceFactory::CreateShader ( IRHIDevice &InDevice,
                                                                 const ERHIShaderStage InStage ) const noexcept
{
    FRHIShaderDesc ShaderDesc;
    ShaderDesc.Stage = InStage;
    return InDevice.CreateShader( ShaderDesc );
}

Lumen::TExpected<Lumen::TSharedRef<Lumen::RHI::IRHISwapchain>, Lumen::RHI::FRHIError>
Lumen::RHI::Tests::FNullConformanceDeviceFactory::CreateSwapchain (
    IRHIDevice &InDevice, const TSharedPtr<IRHISwapchain> &InOldSwapchain ) const noexcept
{
    FRHISwapchainDesc SwapchainDesc;
    SwapchainDesc.Surface.Type          = ERHISurfaceType::Null;
    SwapchainDesc.Surface.NativeWindow  = 1U;
    SwapchainDesc.Surface.InitialExtent = { 320U, 200U, 1U };
    SwapchainDesc.Extent                = SwapchainDesc.Surface.InitialExtent;
    SwapchainDesc.OldSwapchain          = InOldSwapchain;
    return InDevice.CreateSwapchain( SwapchainDesc );
}

Lumen::Bool Lumen::RHI::Tests::FNullConformanceDeviceFactory::InjectDeviceLost ( IRHIDevice &InDevice ) const noexcept
{
    return TryUnwrapRHIBackend<FNullDevice>( &InDevice )
        .Inspect( [] ( FNullDevice *Device ) noexcept
    {
        Device->MarkLost();
    } ).HasValue();
}

Lumen::Bool
Lumen::RHI::Tests::FNullConformanceDeviceFactory::InjectSwapchainOutOfDate ( IRHISwapchain &InSwapchain ) const noexcept
{
    return TryUnwrapRHIBackend<FNullSwapchain>( &InSwapchain )
        .Inspect( [] ( FNullSwapchain *Swapchain ) noexcept
    {
        Swapchain->MarkOutOfDate();
    } ).HasValue();
}

Lumen::RHI::Tests::IRHIConformanceDeviceFactory &Lumen::RHI::Tests::GetRHIConformanceDeviceFactory () noexcept
{
    static FNullConformanceDeviceFactory Factory;
    return Factory;
}
