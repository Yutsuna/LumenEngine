/**
 * @file RenderFrameTypes.hpp
 * @brief Value contracts exchanged with the render executor.
 */

#pragma once

#include <Concepts/TriviallyCopyable.hpp>
#include <Core/CoreTypes.hpp>

namespace Lumen
{

/**
 * @struct FRenderExtent3D
 * @brief Backend-neutral render target extent. No RHI type crosses this boundary.
 */
struct FRenderExtent3D final
{
    UInt32 Width  = 1U;
    UInt32 Height = 1U;
    UInt32 Depth  = 1U;

    [[nodiscard]] constexpr Bool IsValid () const noexcept
    {
        return Width > 0U and Height > 0U and Depth > 0U;
    }

    friend constexpr auto operator<=>( const FRenderExtent3D &, const FRenderExtent3D & ) = default;
};

/**
 * @enum ERenderCommandType
 * @brief Bounded set of commands accepted by the render executor.
 */
enum class ERenderCommandType : UInt8
{
    RenderLatest,
    Resize,
    RecreateDevice,
    Shutdown,
};

/**
 * @struct FRenderFrameRequest
 * @brief Frame index paired with the latest published snapshot sequence.
 */
struct FRenderFrameRequest
{
    UInt64 FrameIndex       = 0ULL;
    UInt64 SnapshotSequence = 0ULL;
};

/**
 * @struct FRenderCommand
 * @brief Trivially copyable render request, resize or control command.
 */
struct FRenderCommand
{
    ERenderCommandType Type = ERenderCommandType::RenderLatest;
    FRenderFrameRequest Frame{};
    FRenderExtent3D Extent{};

    [[nodiscard]] static constexpr FRenderCommand MakeRender ( const FRenderFrameRequest InRequest ) noexcept
    {
        FRenderCommand Command;
        Command.Type  = ERenderCommandType::RenderLatest;
        Command.Frame = InRequest;
        return Command;
    }

    [[nodiscard]] static constexpr FRenderCommand MakeResize ( const FRenderExtent3D InExtent ) noexcept
    {
        FRenderCommand Command;
        Command.Type   = ERenderCommandType::Resize;
        Command.Extent = InExtent;
        return Command;
    }

    [[nodiscard]] static constexpr FRenderCommand MakeSimple ( const ERenderCommandType InType ) noexcept
    {
        FRenderCommand Command;
        Command.Type = InType;
        return Command;
    }
};

/**
 * @enum ERenderFrameResult
 * @brief Observable outcome of one consumed frame command.
 */
enum class ERenderFrameResult : UInt8
{
    Idle,
    Enqueued,
    Completed,
    Dropped,
    QueueFull,
    CapacityExceeded,
    InvalidState,
    DeviceLost,
    DeviceClosed,
    RHIError,
    GraphError,
    Presented,
    SwapchainOutOfDate,
    SwapchainSuboptimal,
    ReadbackReady,
    Shutdown,
};

/**
 * @struct FRenderFrameStatus
 * @brief Latest-only, single-reader frame counters and completion result.
 */
struct FRenderFrameStatus
{
    ERenderFrameResult Result    = ERenderFrameResult::Idle;
    UInt64 LastRequestedFrame    = 0ULL;
    UInt64 LastConsumedFrame     = 0ULL;
    UInt64 LastConsumedSequence  = 0ULL;
    UInt64 LastSubmittedTimeline = 0ULL;
    UInt64 LastCompletedTimeline = 0ULL;
    UInt64 DroppedSnapshotCount  = 0ULL;
    UInt64 RejectedCommandCount  = 0ULL;
    UInt64 PresentCount          = 0ULL;
    UInt64 DrawCallCount         = 0ULL;
    UInt64 ProxyCount            = 0ULL;
    UInt64 ReadbackByteCount     = 0ULL;
    UInt64 ExtensionFrameCount   = 0ULL;
};

static_assert( Concepts::CTriviallyCopyable<FRenderExtent3D> );
static_assert( Concepts::CTriviallyCopyable<FRenderFrameRequest> );
static_assert( Concepts::CTriviallyCopyable<FRenderCommand> );
static_assert( Concepts::CTriviallyCopyable<FRenderFrameStatus> );

} // namespace Lumen
