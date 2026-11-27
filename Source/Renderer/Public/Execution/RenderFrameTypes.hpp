/**
 * @file RenderFrameTypes.hpp
 * @brief Value contracts exchanged with the render executor.
 */

#pragma once

#include <Concepts/TriviallyCopyable.hpp>
#include <Core/CoreTypes.hpp>
#include <RHI/Types/RHIExtent.hpp>

namespace Lumen
{

/** @enum ERenderCommandType */
enum class ERenderCommandType : UInt8
{
    RenderLatest,
    Resize,
    RecreateDevice,
    Shutdown,
};

/** @struct FRenderFrameRequest */
struct FRenderFrameRequest
{
    UInt64 FrameIndex       = 0ULL;
    UInt64 SnapshotSequence = 0ULL;
};

/** @struct FRenderCommand */
struct FRenderCommand
{
    ERenderCommandType Type = ERenderCommandType::RenderLatest;
    FRenderFrameRequest Frame{};
    RHI::FRHIExtent3D Extent{};

    [[nodiscard]] static constexpr FRenderCommand MakeRender ( const FRenderFrameRequest InRequest ) noexcept
    {
        FRenderCommand Command;
        Command.Type  = ERenderCommandType::RenderLatest;
        Command.Frame = InRequest;
        return Command;
    }

    [[nodiscard]] static constexpr FRenderCommand MakeResize ( const RHI::FRHIExtent3D InExtent ) noexcept
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

/** @enum ERenderFrameResult */
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
    Shutdown,
};

/** @struct FRenderFrameStatus */
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
};

static_assert( Concepts::CTriviallyCopyable<FRenderFrameRequest> );
static_assert( Concepts::CTriviallyCopyable<FRenderCommand> );
static_assert( Concepts::CTriviallyCopyable<FRenderFrameStatus> );

} // namespace Lumen
