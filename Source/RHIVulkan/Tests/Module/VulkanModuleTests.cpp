/**
 * @file VulkanModuleTests.cpp
 * @brief Functional tests for the Vulkan RHI module entry point.
 */

#include <RHIVulkan/RHIVulkanModule.hpp>
#include <Tests/UnitTest.hpp>

namespace Lumen
{

namespace RHIVulkan
{

    LUMEN_TEST( VulkanModule, CreateReturnsModule )
    {
        const TExpected<TSharedRef<RHI::IRHIModule>, RHI::FRHIError> Module = CreateRHIVulkanModule();
        LUMEN_CHECK_TRUE( Module.HasValue() );
    }

    LUMEN_TEST( VulkanModule, ApiVersionIsOneZeroZero )
    {
        const TExpected<TSharedRef<RHI::IRHIModule>, RHI::FRHIError> Module = CreateRHIVulkanModule();
        LUMEN_CHECK_TRUE( Module.HasValue() );
        const RHI::FRHIApiVersion Version = ( *Module )->GetApiVersion();
        LUMEN_CHECK_EQ( Version.Major, 1U );
        LUMEN_CHECK_EQ( Version.Minor, 0U );
        LUMEN_CHECK_EQ( Version.Patch, 0U );
    }

    LUMEN_TEST( VulkanModule, EnumerateAdaptersIsUnsupported )
    {
        const TExpected<TSharedRef<RHI::IRHIModule>, RHI::FRHIError> Module = CreateRHIVulkanModule();
        LUMEN_CHECK_TRUE( Module.HasValue() );
        const TExpected<TVector<RHI::FRHIAdapterInfo>, RHI::FRHIError> Adapters = ( *Module )->EnumerateAdapters();
        LUMEN_CHECK_HAS_ERROR( Adapters );
        LUMEN_CHECK_EQ( Adapters.Error().Code, RHI::ERHIErrorCode::Unsupported );
    }

    LUMEN_TEST( VulkanModule, CreateDeviceIsUnsupportedAfterValidation )
    {
        const TExpected<TSharedRef<RHI::IRHIModule>, RHI::FRHIError> Module = CreateRHIVulkanModule();
        LUMEN_CHECK_TRUE( Module.HasValue() );
        const TExpected<TSharedRef<RHI::IRHIDevice>, RHI::FRHIError> Device = ( *Module )->CreateDevice( RHI::FRHIDeviceDesc{} );
        LUMEN_CHECK_HAS_ERROR( Device );
        LUMEN_CHECK_EQ( Device.Error().Code, RHI::ERHIErrorCode::Unsupported );
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
