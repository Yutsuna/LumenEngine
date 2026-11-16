/**
 * @file RHIPresentationConformanceTests.cpp
 * @brief Runs portable swapchain creation, acquisition, presentation, and resize.
 */

#include <Device/IRHIDevice.hpp>
#include <Device/RHIConformanceDeviceFactory.hpp>
#include <Presentation/IRHISwapchain.hpp>
#include <Queue/IRHIQueue.hpp>
#include <Tests/UnitTest.hpp>

namespace Lumen
{
namespace
{

    LUMEN_TEST( RHIPresentationConformance, PresentsAndRecreatesAnImageRing )
    {
        auto Device = Tests::GetRHIConformanceDeviceFactory().CreateDevice();
        LUMEN_CHECK_TRUE( Device.HasValue() );
        if ( not( *Device )->GetCapabilities().Supports( ERHICapabilityFlags::Presentation ) )
        {
            return;
        }
        FRHISwapchainDesc Desc;
        Desc.Surface.Type          = ERHISurfaceType::Null;
        Desc.Surface.NativeWindow  = 1U;
        Desc.Surface.InitialExtent = { 320U, 200U, 1U };
        Desc.Extent                = Desc.Surface.InitialExtent;
        auto Swapchain             = ( *Device )->CreateSwapchain( Desc );
        LUMEN_CHECK_TRUE( Swapchain.HasValue() );
        auto Queue    = *( *Device )->GetQueue( ERHIQueueType::Graphics, 0U );
        auto Acquired = ( *Swapchain )->AcquireNextImage();
        LUMEN_CHECK_TRUE( Acquired.HasValue() );
        LUMEN_CHECK_TRUE( Acquired->Image.Get() != nullptr );
        LUMEN_CHECK_TRUE( ( *Swapchain )->Present( *Queue, *Acquired, nullptr ).HasValue() );
        LUMEN_CHECK_TRUE( ( *Swapchain )->Resize( { 640U, 400U, 1U } ).HasValue() );
        LUMEN_CHECK_EQ( ( *Swapchain )->GetDesc().Extent.Width, 640U );

        Desc.OldSwapchain = *Swapchain;
        auto Replacement  = ( *Device )->CreateSwapchain( Desc );
        LUMEN_CHECK_TRUE( Replacement.HasValue() );
        LUMEN_CHECK_TRUE( ( *Replacement )->GetDesc().OldSwapchain.Get() == ( *Swapchain ).Get() );
    }

} // namespace
} // namespace Lumen
