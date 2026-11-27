/**
 * @file FrameExtensionTests.cpp
 * @brief Integration coverage for custom frame recording and commit-time state.
 */

#include <Execution/Renderer.hpp>

#include <Device/RHIConformanceDeviceFactory.hpp>
#include <RHI/Commands/IRHICommandList.hpp>
#include <RHI/Device/IRHIDevice.hpp>
#include <Rendering/SceneExtractor.hpp>
#include <Tests/UnitTest.hpp>

namespace Lumen
{

namespace
{

    class FFrameExtensionProbe final : public IRenderFrameExtension
    {
    public:
        RHI::FRHIExpected Initialize ( RHI::IRHIDevice &LUMEN_UNUSED InDevice,
                                       const RHI::FRHIExtent3D &InExtent ) noexcept override
        {
            ++InitializeCount;
            Extent = InExtent;
            return {};
        }

        RHI::FRHIExpected Resize ( const RHI::FRHIExtent3D &InExtent ) noexcept override
        {
            ++ResizeCount;
            Extent = InExtent;
            return {};
        }

        RHI::FRHIExpected Record ( const FRenderFrameExtensionContext &InContext ) noexcept override
        {
            ++RecordCount;
            if ( bFailRecord )
            {
                return MakeUnexpected( RHI::MakeRHIInvalidCommand( "Injected extension record failure." ) );
            }

            RHI::FRHIRenderingAttachment Attachment{};
            Attachment.View    = InContext.TargetView;
            Attachment.LoadOp  = RHI::ERHIRenderingLoadOp::DontCare;
            Attachment.StoreOp = RHI::ERHIRenderingStoreOp::Store;
            RHI::FRHIRenderingDesc Desc;
            Desc.Area = RHI::FRHIRect2D{ 0, 0, InContext.Extent.Width, InContext.Extent.Height };
            Desc.ColorAttachments.push_back( Attachment );
            LUMEN_TRY_EXPECT_ERROR( InContext.CommandList.BeginRendering( Desc ) );
            return InContext.CommandList.EndRendering();
        }

        void Commit ( const UInt64 InTimelineValue, FRenderFrameStatus &InOutStatus ) noexcept override
        {
            ++CommitCount;
            LastTimeline                 = InTimelineValue;
            InOutStatus.ExtensionFrameCount = CommitCount;
        }

        RHI::FRHIExtent3D Extent{};
        UInt64 InitializeCount = 0ULL;
        UInt64 ResizeCount     = 0ULL;
        UInt64 RecordCount     = 0ULL;
        UInt64 CommitCount     = 0ULL;
        UInt64 LastTimeline    = 0ULL;
        Bool bFailRecord       = false;
    };

    [[nodiscard]] Bool PublishFrame ( FSceneExtractor &InExtractor ) noexcept
    {
        return InExtractor.Publish( { .WorldId = 91001ULL, .FrameIndex = 1ULL } ).HasValue();
    }

    void RequestFrame ( FRenderer &InRenderer ) noexcept
    {
        for ( UInt32 Attempt = 0U; Attempt < 100000U; ++Attempt )
        {
            if ( InRenderer.RequestRender( 1ULL ) )
            {
                return;
            }
        }
        LUMEN_CHECK_MSG( false, "The extension frame was never accepted" );
    }

    LUMEN_INTEGRATION_TEST( FFrameExtension_Null, RecordsResizesAndCommitsAfterSubmission )
    {
        const auto Device = RHI::Tests::GetRHIConformanceDeviceFactory().CreateDevice( false );
        LUMEN_CHECK_HAS_VALUE( Device );
        if ( not Device )
        {
            return;
        }

        FRenderSnapshotStream Stream( { .TransformCapacity = 1ULL } );
        FSceneExtractor Extractor( Stream );
        LUMEN_CHECK_TRUE( PublishFrame( Extractor ) );
        TSharedPtr<FFrameExtensionProbe> Extension = MakeShared<FFrameExtensionProbe>();
        FRenderer Renderer;
        const FRendererDesc Desc{ .Device          = *Device,
                                  .SnapshotStream  = &Stream,
                                  .Extent          = { 64U, 64U, 1U },
                                  .CommandCapacity = 16ULL,
                                  .Swapchain       = {},
                                  .Shaders         = {},
                                  .ProxyCapacity   = 1ULL,
                                  .AssetCapacity   = 1ULL,
                                  .Readback        = {},
                                  .FrameExtension  = Extension };
        LUMEN_CHECK_HAS_VALUE( Renderer.Start( Desc ) );
        RequestFrame( Renderer );
        LUMEN_CHECK_HAS_VALUE( Renderer.RequestResize( { 32U, 16U, 1U } ) );
        RequestFrame( Renderer );
        Renderer.RequestShutdown();
        Renderer.Join();

        LUMEN_CHECK_EQ( Extension->InitializeCount, 1ULL );
        LUMEN_CHECK_EQ( Extension->ResizeCount, 1ULL );
        LUMEN_CHECK_EQ( Extension->Extent.Width, 32U );
        LUMEN_CHECK_EQ( Extension->Extent.Height, 16U );
        LUMEN_CHECK_EQ( Extension->RecordCount, 2ULL );
        LUMEN_CHECK_EQ( Extension->CommitCount, 2ULL );
        LUMEN_CHECK_TRUE( Extension->LastTimeline >= 2ULL );
        Renderer.SwapStatusBuffers();
        LUMEN_CHECK_EQ( Renderer.GetStatus().ExtensionFrameCount, 2ULL );
    }

    LUMEN_INTEGRATION_TEST( FFrameExtension_Null, RecordFailureSkipsCommit )
    {
        const auto Device = RHI::Tests::GetRHIConformanceDeviceFactory().CreateDevice( false );
        LUMEN_CHECK_HAS_VALUE( Device );
        if ( not Device )
        {
            return;
        }

        FRenderSnapshotStream Stream( { .TransformCapacity = 1ULL } );
        FSceneExtractor Extractor( Stream );
        LUMEN_CHECK_TRUE( PublishFrame( Extractor ) );
        TSharedPtr<FFrameExtensionProbe> Extension = MakeShared<FFrameExtensionProbe>();
        Extension->bFailRecord                     = true;
        FRenderer Renderer;
        const FRendererDesc Desc{ .Device          = *Device,
                                  .SnapshotStream  = &Stream,
                                  .Extent          = { 16U, 16U, 1U },
                                  .CommandCapacity = 16ULL,
                                  .Swapchain       = {},
                                  .Shaders         = {},
                                  .ProxyCapacity   = 1ULL,
                                  .AssetCapacity   = 1ULL,
                                  .Readback        = {},
                                  .FrameExtension  = Extension };
        LUMEN_CHECK_HAS_VALUE( Renderer.Start( Desc ) );
        RequestFrame( Renderer );
        Renderer.RequestShutdown();
        Renderer.Join();
        Renderer.SwapStatusBuffers();

        LUMEN_CHECK_EQ( Extension->RecordCount, 1ULL );
        LUMEN_CHECK_EQ( Extension->CommitCount, 0ULL );
        LUMEN_CHECK_EQ( Renderer.GetStatus().Result, ERenderFrameResult::RHIError );
    }

} // namespace

} // namespace Lumen
