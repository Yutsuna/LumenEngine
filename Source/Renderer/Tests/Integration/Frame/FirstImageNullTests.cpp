/**
 * @file FirstImageNullTests.cpp
 * @brief Deterministic Null coverage of proxies, the first opaque image and presentation ordering.
 */

#include <Backend/StartWithTestDevice.hpp>
#include <Execution/Renderer.hpp>

#include <Frame/RenderFrameContracts.hpp>

#include <HAL/PlatformTime.hpp>

#include <Device/NullFaultPlan.hpp>
#include <Device/RHIConformanceDeviceFactory.hpp>
#include <Presentation/NullPresentation.hpp>
#include <RHI/Device/IRHIDevice.hpp>
#include <RHI/Presentation/IRHISwapchain.hpp>
#include <RHI/Queue/IRHIQueue.hpp>

#include <Rendering/RenderSnapshotStream.hpp>
#include <Rendering/SceneExtractor.hpp>

#include <Tests/UnitTest.hpp>

#include <array>

namespace Lumen
{

namespace
{

    constexpr UInt64 FirstWorldId = 90001ULL;
    constexpr FRenderAssetId QuadAsset{ 7ULL };
    constexpr FRenderProxyId QuadProxy{ 11ULL };
    constexpr FRenderProxyId CameraProxy{ 3ULL };

    [[nodiscard]] FRenderShaderSet MakeNullShaders () noexcept
    {
        return FRenderShaderSet{ .VertexBytecode   = { Byte{ 1U } },
                                 .PixelBytecode    = { Byte{ 1U } },
                                 .VertexEntryPoint = "Main",
                                 .PixelEntryPoint  = "Main",
                                 .Format           = ERenderShaderFormat::Null };
    }

    [[nodiscard]] std::array<Maths::FVec3f, 4U> MakeQuadVertices () noexcept
    {
        return {
            Maths::FVec3f{ -0.5F, -0.5F, 0.0F },
            Maths::FVec3f{ 0.5F, -0.5F, 0.0F },
            Maths::FVec3f{ 0.5F, 0.5F, 0.0F },
            Maths::FVec3f{ -0.5F, 0.5F, 0.0F },
        };
    }

    [[nodiscard]] std::array<UInt32, 6U> MakeQuadIndices () noexcept
    {
        return { 0U, 1U, 2U, 0U, 2U, 3U };
    }

    [[nodiscard]] Bool PublishQuad ( FSceneExtractor &InExtractor, const Bool bIncludeMesh, const UInt64 InFrameIndex ) noexcept
    {
        const std::array<FRenderTransformRecord, 2U> Transforms = {
            FRenderTransformRecord{ .ProxyId = CameraProxy, .Flags = ERenderTransformFlags::Visible },
            FRenderTransformRecord{ .ProxyId = QuadProxy, .Flags = ERenderTransformFlags::Visible },
        };
        const FRenderMeshRecord Mesh{ .ProxyId         = QuadProxy,
                                      .MeshAssetId     = QuadAsset,
                                      .MaterialAssetId = FRenderAssetId( 8ULL ),
                                      .TransformIndex  = 1ULL,
                                      .Flags           = ERenderMeshFlags::Visible };
        const FRenderCameraRecord Camera{ .ProxyId = CameraProxy, .Flags = ERenderCameraFlags::Active };
        const std::array<FRenderMeshRecord, 1U> MeshStorage = { Mesh };
        const TSpan<const FRenderMeshRecord> Meshes =
            bIncludeMesh ? TSpan<const FRenderMeshRecord>( MeshStorage ) : TSpan<const FRenderMeshRecord>{};
        return InExtractor
            .Publish( { .WorldId    = FirstWorldId,
                        .FrameIndex = InFrameIndex,
                        .Transforms = Transforms,
                        .Meshes     = Meshes,
                        .Cameras    = TSpan<const FRenderCameraRecord>( &Camera, 1U ) } )
            .HasValue();
    }

    void RequestFrame ( FRenderer &InRenderer, const UInt64 InFrameIndex )
    {
        const Float64 Deadline = HAL::FPlatformTime::Seconds() + 2.0;
        while ( HAL::FPlatformTime::Seconds() < Deadline )
        {
            if ( InRenderer.RequestRender( InFrameIndex ).HasValue() )
            {
                return;
            }
            InRenderer.SwapStatusBuffers();
            HAL::FPlatformTime::Sleep( 0.001 );
        }
        LUMEN_CHECK_MSG( false, "Render request was never accepted" );
    }

    void WaitForFrame ( FRenderer &InRenderer, const UInt64 InFrameIndex )
    {
        const Float64 Deadline = HAL::FPlatformTime::Seconds() + 2.0;
        while ( HAL::FPlatformTime::Seconds() < Deadline )
        {
            InRenderer.SwapStatusBuffers();
            const FRenderFrameStatus &Status = InRenderer.GetStatus();
            if ( ( Status.LastConsumedFrame >= InFrameIndex and Status.LastSubmittedTimeline > 0ULL ) or
                 Status.Result == ERenderFrameResult::RHIError or Status.Result == ERenderFrameResult::GraphError or
                 Status.Result == ERenderFrameResult::CapacityExceeded or Status.Result == ERenderFrameResult::DeviceLost )
            {
                return;
            }
            HAL::FPlatformTime::Sleep( 0.001 );
        }
        LUMEN_CHECK_MSG( false, "Frame was never submitted" );
    }

    [[nodiscard]] Bool StartForwardRenderer ( FRenderer &InRenderer,
                                              const TSharedRef<RHI::IRHIDevice> &InDevice,
                                              FRenderSnapshotStream &InStream,
                                              const TSharedPtr<RHI::IRHISwapchain> &InSwapchain = {} )
    {
        const auto Vertices = MakeQuadVertices();
        const auto Indices  = MakeQuadIndices();
        LUMEN_CHECK_HAS_VALUE( InRenderer.RegisterGeometry( { .Id = QuadAsset, .Positions = Vertices, .Indices = Indices } ) );
        const FRendererDesc Desc{
            .SnapshotStream  = &InStream,
            .Extent          = { 64U, 64U, 1U },
            .Surface         = {},
            .Backend         = "null",
            .CommandCapacity = 64ULL,
            .Shaders         = MakeNullShaders(),
            .ProxyCapacity   = 32ULL,
            .AssetCapacity   = 8ULL,
            .Readback        = InSwapchain ? FRenderFrameReadbackDesc{} : FRenderFrameReadbackDesc{ 64ULL * 64ULL * 4ULL },
            .Feature         = {},
        };
        LUMEN_CHECK_HAS_VALUE( StartWithTestDevice( InRenderer, InDevice, Desc, InSwapchain ) );
        return InRenderer.IsRunning();
    }

    LUMEN_INTEGRATION_TEST( FFirstImage_Null, DrawsIndexedQuadAndExposesReadback )
    {
        RHI::Tests::IRHIConformanceDeviceFactory &Factory = RHI::Tests::GetRHIConformanceDeviceFactory();
        const auto DeviceResult                           = Factory.CreateDevice( false );
        LUMEN_CHECK_HAS_VALUE( DeviceResult );
        if ( not DeviceResult )
        {
            return;
        }

        FRenderSnapshotStream Stream(
            { .TransformCapacity = 8ULL, .MeshCapacity = 8ULL, .CameraCapacity = 8ULL, .LightCapacity = 8ULL } );
        FSceneExtractor Extractor( Stream );
        LUMEN_CHECK_TRUE( PublishQuad( Extractor, true, 1ULL ) );

        FRenderer Renderer;
        if ( not StartForwardRenderer( Renderer, *DeviceResult, Stream ) )
        {
            return;
        }

        RequestFrame( Renderer, 1ULL );
        WaitForFrame( Renderer, 1ULL );
        Renderer.SwapStatusBuffers();

        const FRenderFrameStatus &Status = Renderer.GetStatus();
        LUMEN_CHECK_TRUE( Status.LastSubmittedTimeline >= 1ULL );
        LUMEN_CHECK_EQ( Status.DrawCallCount, 1ULL );
        LUMEN_CHECK_EQ( Status.ProxyCount, 1ULL );
        LUMEN_CHECK_EQ( Status.Result, ERenderFrameResult::ReadbackReady );
        const auto Readback = Renderer.GetReadbackData();
        LUMEN_CHECK_TRUE( Readback.HasValue() );
        if ( Readback )
        {
            LUMEN_CHECK_EQ( Readback->size(), 64ULL * 64ULL * 4ULL );
        }
        LUMEN_CHECK_EQ( Status.ReadbackByteCount, 64ULL * 64ULL * 4ULL );
        Renderer.RequestShutdown();
        Renderer.Join();
    }

    LUMEN_INTEGRATION_TEST( FFirstImage_Null, PresentsAcquiredImageInAcquireSubmitOrder )
    {
        RHI::Tests::IRHIConformanceDeviceFactory &Factory = RHI::Tests::GetRHIConformanceDeviceFactory();
        const auto DeviceResult                           = Factory.CreateDevice( false );
        LUMEN_CHECK_HAS_VALUE( DeviceResult );
        if ( not DeviceResult )
        {
            return;
        }
        const auto SwapchainResult = Factory.CreateSwapchain( **DeviceResult );
        LUMEN_CHECK_HAS_VALUE( SwapchainResult );
        if ( not SwapchainResult )
        {
            return;
        }

        FRenderSnapshotStream Stream(
            { .TransformCapacity = 8ULL, .MeshCapacity = 8ULL, .CameraCapacity = 8ULL, .LightCapacity = 8ULL } );
        FSceneExtractor Extractor( Stream );
        LUMEN_CHECK_TRUE( PublishQuad( Extractor, true, 1ULL ) );

        FRenderer Renderer;
        if ( not StartForwardRenderer( Renderer, *DeviceResult, Stream, *SwapchainResult ) )
        {
            return;
        }

        RequestFrame( Renderer, 1ULL );
        WaitForFrame( Renderer, 1ULL );
        Renderer.SwapStatusBuffers();
        LUMEN_CHECK_EQ( Renderer.GetStatus().PresentCount, 1ULL );
        LUMEN_CHECK_EQ( Renderer.GetStatus().Result, ERenderFrameResult::Presented );

        Renderer.RequestShutdown();
        Renderer.Join();
    }

    LUMEN_INTEGRATION_TEST( FFirstImage_Null, RemovesRetiredProxyAfterSkippedFrames )
    {
        RHI::Tests::IRHIConformanceDeviceFactory &Factory = RHI::Tests::GetRHIConformanceDeviceFactory();
        const auto DeviceResult                           = Factory.CreateDevice( false );
        LUMEN_CHECK_HAS_VALUE( DeviceResult );
        if ( not DeviceResult )
        {
            return;
        }

        FRenderSnapshotStream Stream(
            { .TransformCapacity = 8ULL, .MeshCapacity = 8ULL, .CameraCapacity = 8ULL, .LightCapacity = 8ULL } );
        FSceneExtractor Extractor( Stream );
        LUMEN_CHECK_TRUE( PublishQuad( Extractor, true, 1ULL ) );
        LUMEN_CHECK_TRUE( PublishQuad( Extractor, false, 2ULL ) );
        LUMEN_CHECK_TRUE( PublishQuad( Extractor, false, 3ULL ) );

        FRenderer Renderer;
        if ( not StartForwardRenderer( Renderer, *DeviceResult, Stream ) )
        {
            return;
        }

        RequestFrame( Renderer, 3ULL );
        WaitForFrame( Renderer, 3ULL );
        Renderer.RequestShutdown();
        Renderer.Join();
        Renderer.SwapStatusBuffers();

        const FRenderFrameStatus &Status = Renderer.GetStatus();
        LUMEN_CHECK_EQ( Status.LastConsumedSequence, 3ULL );
        LUMEN_CHECK_EQ( Status.ProxyCount, 0ULL );
        LUMEN_CHECK_EQ( Status.DrawCallCount, 0ULL );
        LUMEN_CHECK_TRUE( Status.DroppedSnapshotCount >= 1ULL );
    }

    LUMEN_INTEGRATION_TEST( FFirstImage_Null, ValidationAcceptsGraphBarriersAndIndexedDraw )
    {
        RHI::Tests::IRHIConformanceDeviceFactory &Factory = RHI::Tests::GetRHIConformanceDeviceFactory();
        const auto DeviceResult                           = Factory.CreateDevice( true );
        LUMEN_CHECK_HAS_VALUE( DeviceResult );
        if ( not DeviceResult )
        {
            return;
        }

        FRenderSnapshotStream Stream(
            { .TransformCapacity = 8ULL, .MeshCapacity = 8ULL, .CameraCapacity = 8ULL, .LightCapacity = 8ULL } );
        FSceneExtractor Extractor( Stream );
        LUMEN_CHECK_TRUE( PublishQuad( Extractor, true, 1ULL ) );

        FRenderer Renderer;
        if ( not StartForwardRenderer( Renderer, *DeviceResult, Stream ) )
        {
            return;
        }
        RequestFrame( Renderer, 1ULL );
        WaitForFrame( Renderer, 1ULL );
        Renderer.RequestShutdown();
        Renderer.Join();
        Renderer.SwapStatusBuffers();

        LUMEN_CHECK_EQ( Renderer.GetStatus().DrawCallCount, 1ULL );
        LUMEN_CHECK_NOT_EQ( Renderer.GetStatus().Result, ERenderFrameResult::RHIError );
        LUMEN_CHECK_NOT_EQ( Renderer.GetStatus().Result, ERenderFrameResult::InvalidState );
    }

    LUMEN_INTEGRATION_TEST( FFirstImage_Null, DeviceLossStopsPresentation )
    {
        RHI::Tests::IRHIConformanceDeviceFactory &Factory = RHI::Tests::GetRHIConformanceDeviceFactory();
        const auto DeviceResult                           = Factory.CreateDevice( false );
        LUMEN_CHECK_HAS_VALUE( DeviceResult );
        if ( not DeviceResult )
        {
            return;
        }
        const auto SwapchainResult = Factory.CreateSwapchain( **DeviceResult );
        LUMEN_CHECK_HAS_VALUE( SwapchainResult );
        if ( not SwapchainResult )
        {
            return;
        }

        FRenderSnapshotStream Stream(
            { .TransformCapacity = 8ULL, .MeshCapacity = 8ULL, .CameraCapacity = 8ULL, .LightCapacity = 8ULL } );
        FSceneExtractor Extractor( Stream );
        LUMEN_CHECK_TRUE( PublishQuad( Extractor, true, 1ULL ) );

        FRenderer Renderer;
        if ( not StartForwardRenderer( Renderer, *DeviceResult, Stream, *SwapchainResult ) )
        {
            return;
        }
        LUMEN_CHECK_TRUE( Factory.InjectDeviceLost( **DeviceResult ) );

        RequestFrame( Renderer, 1ULL );
        WaitForFrame( Renderer, 1ULL );
        Renderer.SwapStatusBuffers();
        LUMEN_CHECK_EQ( Renderer.GetStatus().Result, ERenderFrameResult::DeviceLost );
        LUMEN_CHECK_EQ( Renderer.GetStatus().PresentCount, 0ULL );
        Renderer.RequestShutdown();
        Renderer.Join();
    }

} // namespace

} // namespace Lumen
