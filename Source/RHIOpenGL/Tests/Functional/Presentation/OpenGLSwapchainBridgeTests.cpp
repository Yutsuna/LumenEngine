/**
 * @file OpenGLSwapchainBridgeTests.cpp
 * @brief Functional coverage for windowed presentation through IRHISurfaceBridge.
 */

#include "Device/RHIConformanceDeviceFactory.hpp"

#include <RHI/Device/IRHIDevice.hpp>
#include <RHI/Presentation/IRHISurfaceBridge.hpp>
#include <RHI/Presentation/IRHISwapchain.hpp>
#include <RHI/Presentation/RHIAcquiredImage.hpp>
#include <RHI/Presentation/RHISwapchainDesc.hpp>
#include <RHI/Queue/IRHIQueue.hpp>

#include <Tests/UnitTest.hpp>

namespace Lumen
{

namespace
{

    using namespace RHI;

    class FRecordingSurfaceBridge final : public IRHISurfaceBridge
    {
    public:

        [[nodiscard]] TExpected<FRHINativeSurface, FRHIError> CreateNativeSurface ( const UPtr InBackendHandle ) noexcept override
        {
            ++CreateCount;
            return FRHINativeSurface{ .BackendHandle = InBackendHandle, .SurfaceHandle = 1U, .ContextHandle = 2U };
        }
        void DestroyNativeSurface ( const FRHINativeSurface & ) noexcept override
        {
            ++DestroyCount;
        }
        [[nodiscard]] FRHIExpected MakeCurrent ( const FRHINativeSurface & ) noexcept override
        {
            ++MakeCurrentCount;
            return {};
        }
        [[nodiscard]] FRHIExpected Present ( const FRHINativeSurface &, const Int32 InSwapInterval ) noexcept override
        {
            ++PresentCount;
            LastSwapInterval = InSwapInterval;
            return {};
        }
        [[nodiscard]] TExpected<FRHIExtent3D, FRHIError> GetExtent () const noexcept override
        {
            return FRHIExtent3D{ 320U, 200U, 1U };
        }
        [[nodiscard]] TOptional<UPtr> ResolveProcedure ( const FStringView ) noexcept override
        {
            return {};
        }

        UInt32 CreateCount      = 0U;
        UInt32 DestroyCount     = 0U;
        UInt32 MakeCurrentCount = 0U;
        UInt32 PresentCount     = 0U;
        Int32 LastSwapInterval  = -1;
    };

    [[nodiscard]] FRHISwapchainDesc MakeWindowDesc ( const TSharedPtr<FRecordingSurfaceBridge> &InBridge,
                                                     const ERHIPresentMode InMode ) noexcept
    {
        FRHISwapchainDesc Desc;
        Desc.Surface.Type          = ERHISurfaceType::Window;
        Desc.Surface.NativeWindow  = 7U;
        Desc.Surface.Bridge        = StaticCastSharedPtr<IRHISurfaceBridge>( InBridge );
        Desc.Surface.InitialExtent = { 320U, 200U, 1U };
        Desc.Extent                = Desc.Surface.InitialExtent;
        Desc.ImageCount            = 2U;
        Desc.PresentMode           = InMode;
        return Desc;
    }

    LUMEN_TEST( OpenGLSwapchainBridge, WindowFifoAndImmediatePresentThroughBridge )
    {
        const TExpected<TSharedRef<IRHIDevice>, FRHIError> Device = RHI::Tests::GetRHIConformanceDeviceFactory().CreateDevice();
        LUMEN_CHECK_TRUE( Device.HasValue() );
        TSharedRef<IRHIQueue> Queue = *( *Device )->GetQueue( ERHIQueueType::Graphics, 0U );

        for ( const ERHIPresentMode Mode : { ERHIPresentMode::Fifo, ERHIPresentMode::Immediate } )
        {
            TSharedPtr<FRecordingSurfaceBridge> Bridge = MakeShared<FRecordingSurfaceBridge>();
            const TExpected<TSharedRef<IRHISwapchain>, FRHIError> Swapchain =
                ( *Device )->CreateSwapchain( MakeWindowDesc( Bridge, Mode ) );
            LUMEN_CHECK_TRUE( Swapchain.HasValue() );
            LUMEN_CHECK_EQ( Bridge->CreateCount, 1U );
            LUMEN_CHECK_TRUE( ( *Swapchain )->GetImageCount() >= 2U );

            const TExpected<FRHIAcquiredImage, FRHIError> Image = ( *Swapchain )->AcquireNextImage();
            LUMEN_CHECK_TRUE( Image.HasValue() );
            LUMEN_CHECK_TRUE( ( *Swapchain )->Present( *Queue, *Image, nullptr ).HasValue() );
            LUMEN_CHECK_EQ( Bridge->PresentCount, 1U );
            LUMEN_CHECK_EQ( Bridge->LastSwapInterval, Mode == ERHIPresentMode::Fifo ? 1 : 0 );
        }
    }

    LUMEN_TEST( OpenGLSwapchainBridge, ResizeRequiresReleasedImageAndMailboxIsRejected )
    {
        const TExpected<TSharedRef<IRHIDevice>, FRHIError> Device = RHI::Tests::GetRHIConformanceDeviceFactory().CreateDevice();
        LUMEN_CHECK_TRUE( Device.HasValue() );
        TSharedRef<IRHIQueue> Queue = *( *Device )->GetQueue( ERHIQueueType::Graphics, 0U );

        TSharedPtr<FRecordingSurfaceBridge> Bridge = MakeShared<FRecordingSurfaceBridge>();
        const TExpected<TSharedRef<IRHISwapchain>, FRHIError> Swapchain =
            ( *Device )->CreateSwapchain( MakeWindowDesc( Bridge, ERHIPresentMode::Fifo ) );
        LUMEN_CHECK_TRUE( Swapchain.HasValue() );

        const TExpected<FRHIAcquiredImage, FRHIError> Image = ( *Swapchain )->AcquireNextImage();
        LUMEN_CHECK_TRUE( Image.HasValue() );
        LUMEN_CHECK_HAS_ERROR( ( *Swapchain )->Resize( { 640U, 480U, 1U } ) );
        LUMEN_CHECK_TRUE( ( *Swapchain )->Present( *Queue, *Image, nullptr ).HasValue() );
        LUMEN_CHECK_TRUE( ( *Swapchain )->Resize( { 640U, 480U, 1U } ).HasValue() );
        LUMEN_CHECK_TRUE( ( *Swapchain )->GetImageCount() >= 2U );

        const TExpected<TSharedRef<IRHISwapchain>, FRHIError> Mailbox =
            ( *Device )->CreateSwapchain( MakeWindowDesc( Bridge, ERHIPresentMode::Mailbox ) );
        LUMEN_CHECK_HAS_ERROR( Mailbox );
        LUMEN_CHECK_EQ( Mailbox.Error().Code, ERHIErrorCode::Unsupported );
    }

} // namespace

} // namespace Lumen
