/**
 * @file NullConformanceDeviceFactory.cpp
 * @brief Implements Null conformance device creation.
 */

#include "Device/NullConformanceDeviceFactory.hpp"
#include "Module/NullModule.hpp"
#include <Device/NullDevice.hpp>
#include <Presentation/NullPresentation.hpp>
#include <Presentation/RHISwapchainDesc.hpp>
#include <Shaders/RHIShaderDesc.hpp>
#include <Templates/Optional.hpp>
#include <Validation/RHIValidation.hpp>

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
    TExpected<TSharedRef<IRHIDevice>, FRHIError> Device = GetModule()->CreateDevice( FRHIDeviceDesc{} );
    if ( not Device or not bEnableValidation )
    {
        return Device;
    }
    return WrapRHIDevice( *Device );
}

Lumen::TExpected<Lumen::TSharedRef<Lumen::IRHIShader>, Lumen::FRHIError>
Lumen::Tests::FNullConformanceDeviceFactory::CreateShader ( IRHIDevice &InDevice, const ERHIShaderStage InStage ) const noexcept
{
    FRHIShaderDesc ShaderDesc;
    ShaderDesc.Stage = InStage;
    return InDevice.CreateShader( ShaderDesc );
}

Lumen::TExpected<Lumen::TSharedRef<Lumen::IRHISwapchain>, Lumen::FRHIError>
Lumen::Tests::FNullConformanceDeviceFactory::CreateSwapchain ( IRHIDevice &InDevice,
                                                               const TSharedPtr<IRHISwapchain> &InOldSwapchain ) const noexcept
{
    FRHISwapchainDesc SwapchainDesc;
    SwapchainDesc.Surface.Type          = ERHISurfaceType::Null;
    SwapchainDesc.Surface.NativeWindow  = 1U;
    SwapchainDesc.Surface.InitialExtent = { 320U, 200U, 1U };
    SwapchainDesc.Extent                = SwapchainDesc.Surface.InitialExtent;
    SwapchainDesc.OldSwapchain          = InOldSwapchain;
    return InDevice.CreateSwapchain( SwapchainDesc );
}

Lumen::Bool Lumen::Tests::FNullConformanceDeviceFactory::InjectDeviceLost ( IRHIDevice &InDevice ) const noexcept
{
    return TryUnwrapRHIBackend<FNullDevice>( &InDevice )
        .Inspect( [] ( FNullDevice *Device ) noexcept
    {
        Device->MarkLost();
    } ).HasValue();
}

Lumen::Bool Lumen::Tests::FNullConformanceDeviceFactory::InjectSwapchainOutOfDate ( IRHISwapchain &InSwapchain ) const noexcept
{
    return TryUnwrapRHIBackend<FNullSwapchain>( &InSwapchain )
        .Inspect( [] ( FNullSwapchain *Swapchain ) noexcept
    {
        Swapchain->MarkOutOfDate();
    } ).HasValue();
}

Lumen::Tests::IRHIConformanceDeviceFactory &Lumen::Tests::GetRHIConformanceDeviceFactory () noexcept
{
    static FNullConformanceDeviceFactory Factory;
    return Factory;
}
