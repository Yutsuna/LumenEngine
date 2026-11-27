/**
 * @file VulkanModuleTests.cpp
 * @brief Functional tests for the Vulkan RHI module entry point.
 */

#include <RHI/Device/IRHIDevice.hpp>
#include <RHIVulkan/RHIVulkanModule.hpp>
#include <Tests/UnitTest.hpp>

namespace Lumen
{

namespace RHIVulkan
{

    LUMEN_TEST( VulkanBootstrap, CreateReturnsModule )
    {
        const TExpected<TSharedRef<RHI::IRHIModule>, RHI::FRHIError> Module = CreateRHIVulkanModule();
        LUMEN_CHECK_TRUE( Module.HasValue() );
    }

    LUMEN_TEST( VulkanBootstrap, ApiVersionIsOneOneZero )
    {
        const TExpected<TSharedRef<RHI::IRHIModule>, RHI::FRHIError> Module = CreateRHIVulkanModule();
        LUMEN_CHECK_TRUE( Module.HasValue() );
        const RHI::FRHIApiVersion Version = ( *Module )->GetApiVersion();
        LUMEN_CHECK_EQ( Version.Major, 1U );
        LUMEN_CHECK_EQ( Version.Minor, 1U );
        LUMEN_CHECK_EQ( Version.Patch, 0U );
    }

    LUMEN_TEST( VulkanModule, EnumerateAdaptersReturnsValidInformation )
    {
        const TExpected<TSharedRef<RHI::IRHIModule>, RHI::FRHIError> Module = CreateRHIVulkanModule();
        LUMEN_CHECK_TRUE( Module.HasValue() );
        const TExpected<TVector<RHI::FRHIAdapterInfo>, RHI::FRHIError> Adapters = ( *Module )->EnumerateAdapters();
        LUMEN_CHECK_TRUE( Adapters.HasValue() );

        for ( const RHI::FRHIAdapterInfo &Adapter : *Adapters )
        {
            LUMEN_CHECK_TRUE( Adapter.Validate().HasValue() );
        }
    }

    LUMEN_TEST( VulkanModule, CreateDeviceReturnsReadyDevice )
    {
        const TExpected<TSharedRef<RHI::IRHIModule>, RHI::FRHIError> Module = CreateRHIVulkanModule();
        LUMEN_CHECK_TRUE( Module.HasValue() );
        const TExpected<TSharedRef<RHI::IRHIDevice>, RHI::FRHIError> Device = ( *Module )->CreateDevice( RHI::FRHIDeviceDesc{} );
        LUMEN_CHECK_TRUE( Device.HasValue() );

        LUMEN_CHECK_EQ( ( *Device )->GetStatus(), RHI::ERHIDeviceStatus::Ready );
    }

    LUMEN_TEST( VulkanModule, CreateDeviceRejectsInvalidDescriptor )
    {
        const TExpected<TSharedRef<RHI::IRHIModule>, RHI::FRHIError> Module = CreateRHIVulkanModule();
        LUMEN_CHECK_TRUE( Module.HasValue() );

        RHI::FRHIDeviceDesc InvalidDesc;
        InvalidDesc.Queues.clear();

        const TExpected<TSharedRef<RHI::IRHIDevice>, RHI::FRHIError> Device = ( *Module )->CreateDevice( InvalidDesc );
        LUMEN_CHECK_HAS_ERROR( Device );
        LUMEN_CHECK_EQ( Device.Error().Code, RHI::ERHIErrorCode::InvalidArgument );
    }

} // namespace RHIVulkan

} // namespace Lumen
