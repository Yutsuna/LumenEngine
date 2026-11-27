/**
 * @file RenderFrameTypesTests.cpp
 * @brief Tests for render executor value contracts.
 */

#include <Execution/RenderFrameTypes.hpp>
#include <Tests/UnitTest.hpp>

namespace Lumen
{

LUMEN_TEST( FRenderCommand, MakeRenderCarriesFrameIdentity )
{
    constexpr FRenderFrameRequest Request{ .FrameIndex = 42ULL, .SnapshotSequence = 17ULL };
    constexpr FRenderCommand Command = FRenderCommand::MakeRender( Request );

    LUMEN_CHECK_EQ( Command.Type, ERenderCommandType::RenderLatest );
    LUMEN_CHECK_EQ( Command.Frame.FrameIndex, 42ULL );
    LUMEN_CHECK_EQ( Command.Frame.SnapshotSequence, 17ULL );
}

LUMEN_TEST( FRenderCommand, MakeResizeCarriesExtent )
{
    constexpr RHI::FRHIExtent3D Extent{ 1920U, 1080U, 1U };
    constexpr FRenderCommand Command = FRenderCommand::MakeResize( Extent );

    LUMEN_CHECK_EQ( Command.Type, ERenderCommandType::Resize );
    LUMEN_CHECK_EQ( Command.Extent.Width, 1920U );
    LUMEN_CHECK_EQ( Command.Extent.Height, 1080U );
    LUMEN_CHECK_EQ( Command.Extent.Depth, 1U );
    LUMEN_CHECK_EQ( Command.Frame.FrameIndex, 0ULL );
}

LUMEN_TEST( FRenderFrameStatus, DefaultsToIdleWithoutInFlightWork )
{
    constexpr FRenderFrameStatus Status;

    LUMEN_CHECK_EQ( Status.Result, ERenderFrameResult::Idle );
    LUMEN_CHECK_EQ( Status.LastConsumedSequence, 0ULL );
    LUMEN_CHECK_EQ( Status.LastSubmittedTimeline, 0ULL );
    LUMEN_CHECK_EQ( Status.DroppedSnapshotCount, 0ULL );
    LUMEN_CHECK_EQ( Status.RejectedCommandCount, 0ULL );
}

} // namespace Lumen
