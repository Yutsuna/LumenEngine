/**
 * @file FrameExtensionTests.cpp
 * @brief Integration coverage for custom frame recording and commit-time state.
 */

#include <Backend/StartWithTestDevice.hpp>
#include <Execution/Renderer.hpp>

#include <HAL/PlatformTime.hpp>

#include <Device/RHIConformanceDeviceFactory.hpp>
#include <RHI/Device/IRHIDevice.hpp>
#include <Rendering/RenderSnapshotStream.hpp>
#include <Rendering/SceneExtractor.hpp>
#include <Tests/UnitTest.hpp>

namespace Lumen
{

namespace
{

    class FFrameExtensionProbe final : public IRenderFeature
    {
    public:

        FRenderExtensionExpected Initialize ( IRenderFeatureHost LUMEN_UNUSED &InHost,
                                              const FRenderExtent3D &InExtent ) noexcept override
        {
            ++InitializeCount;
            Extent = InExtent;
            return {};
        }

        FRenderExtensionExpected Resize ( const FRenderExtent3D &InExtent ) noexcept override
        {
            ++ResizeCount;
            Extent = InExtent;
            return {};
        }

        FRenderExtensionExpected Record ( const FRenderFrameExtensionContext &InContext ) noexcept override
        {
            ++RecordCount;
            static_cast<void>( InContext );
            if ( bFailRecord )
            {
                return MakeUnexpected( FRenderExtensionError{ ERenderExtensionErrorCode::RecordFailed,
                                                              FString( "Injected extension record failure." ) } );
            }
            return {};
        }

        void Commit ( const UInt64 InTimelineValue, FRenderFrameStatus &InOutStatus ) noexcept override
        {
            ++CommitCount;
            LastTimeline                    = InTimelineValue;
            InOutStatus.ExtensionFrameCount = CommitCount;
        }

        FRenderExtent3D Extent{};
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

    void RequestFrame ( FRenderer &InRenderer )
    {
        const Float64 Deadline = HAL::FPlatformTime::Seconds() + 2.0;
        while ( HAL::FPlatformTime::Seconds() < Deadline )
        {
            if ( InRenderer.RequestRender( 1ULL ) )
            {
                return;
            }
            HAL::FPlatformTime::Sleep( 0.001 );
        }
        LUMEN_CHECK_MSG( false, "The extension frame was never accepted" );
    }

    void WaitForRecordFailure ( FRenderer &InRenderer )
    {
        const Float64 Deadline = HAL::FPlatformTime::Seconds() + 2.0;
        while ( HAL::FPlatformTime::Seconds() < Deadline )
        {
            InRenderer.SwapStatusBuffers();
            if ( InRenderer.GetStatus().Result == ERenderFrameResult::RHIError )
            {
                return;
            }
            HAL::FPlatformTime::Sleep( 0.001 );
        }
        LUMEN_CHECK_MSG( false, "The extension record failure was never reported" );
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
        const FRendererDesc Desc{ .SnapshotStream  = &Stream,
                                  .Extent          = { 64U, 64U, 1U },
                                  .Surface         = {},
                                  .Backend         = "null",
                                  .CommandCapacity = 16ULL,
                                  .Shaders         = {},
                                  .ProxyCapacity   = 1ULL,
                                  .AssetCapacity   = 1ULL,
                                  .Readback        = {},
                                  .Feature         = StaticCastSharedPtr<IRenderFeature>( Extension ) };
        LUMEN_CHECK_HAS_VALUE( StartWithTestDevice( Renderer, *Device, Desc ) );
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
        const FRendererDesc Desc{ .SnapshotStream  = &Stream,
                                  .Extent          = { 16U, 16U, 1U },
                                  .Surface         = {},
                                  .Backend         = "null",
                                  .CommandCapacity = 16ULL,
                                  .Shaders         = {},
                                  .ProxyCapacity   = 1ULL,
                                  .AssetCapacity   = 1ULL,
                                  .Readback        = {},
                                  .Feature         = StaticCastSharedPtr<IRenderFeature>( Extension ) };
        LUMEN_CHECK_HAS_VALUE( StartWithTestDevice( Renderer, *Device, Desc ) );
        RequestFrame( Renderer );
        /** INFO: Shutdown publishes its own terminal result, so observe the record failure first. */
        WaitForRecordFailure( Renderer );
        LUMEN_CHECK_EQ( Renderer.GetStatus().Result, ERenderFrameResult::RHIError );
        Renderer.RequestShutdown();
        Renderer.Join();

        LUMEN_CHECK_EQ( Extension->RecordCount, 1ULL );
        LUMEN_CHECK_EQ( Extension->CommitCount, 0ULL );
    }

} // namespace

} // namespace Lumen
