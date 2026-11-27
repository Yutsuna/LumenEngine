/**
 * @file Renderer.hpp
 * @brief Backend-neutral renderer facade driving one render executor.
 */

#pragma once

#include <RendererExport.hpp>

#include <Core/NonCopyable.hpp>
#include <Core/NonMovable.hpp>

#include <Execution/RenderExecutor.hpp>

#include <Memory/SharedPtr.hpp>
#include <Templates/Optional.hpp>

#include <RHI/Commands/RHIRenderingDesc.hpp>
#include <RHI/Types/RHIExtent.hpp>

namespace Lumen
{

namespace RHI
{
    class IRHICommandAllocator;
    class IRHICommandList;
    class IRHIDevice;
    class IRHIQueue;
    class IRHISemaphore;
    class IRHITexture;
    class IRHITextureView;
} // namespace RHI

class FRenderSnapshotStream;

/**
 * @enum ERendererError
 * @brief Failures reported by the public renderer facade.
 */
enum class ERendererError : UInt8
{
    InvalidDesc,
    AlreadyRunning,
    NotRunning,
    CapabilityUnsupported,
    RHICreationFailed,
    ExecutorFailed,
    QueueFull,
};

/**
 * @struct FRendererDesc
 * @brief Composition-root inputs; no backend factory is accepted.
 */
struct RENDERER_EXPORT FRendererDesc final
{
    TSharedPtr<RHI::IRHIDevice> Device;
    FRenderSnapshotStream *SnapshotStream = nullptr;
    RHI::FRHIExtent3D Extent{ 64U, 64U, 1U };
    USize CommandCapacity = 64ULL;
};

/**
 * @class FRenderer
 * @brief Owns the render thread and turns render commands into portable RHI submissions.
 * @details The only Scene surface consumed is FRenderSnapshotStream from Public/Rendering. The render executor is the
 *          sole consumer of commands, the sole reader that swaps the snapshot stream, and the sole recorder and
 *          submitter of RHI command lists. Concrete backends are selected by composition, never here.
 */
class RENDERER_EXPORT FRenderer final : public IRenderCommandHandler, public FNonCopyable, public FNonMovable
{
public:

    FRenderer () noexcept = default;
    ~FRenderer () noexcept override;

    /** @brief Creates the render resources and starts one render thread. */
    [[nodiscard]] TExpected<void, ERendererError> Start ( const FRendererDesc &InDesc ) noexcept;
    /** @brief Enqueues a latest-snapshot render request. */
    [[nodiscard]] TExpected<void, ERendererError> RequestRender ( UInt64 InFrameIndex ) noexcept;
    /** @brief Enqueues a resize applied on the render thread. */
    [[nodiscard]] TExpected<void, ERendererError> RequestResize ( const RHI::FRHIExtent3D &InExtent ) noexcept;
    /** @brief Drains queued commands, then stops and joins the render thread. */
    void RequestShutdown () noexcept;
    void Join () noexcept;
    [[nodiscard]] Bool IsRunning () const noexcept;

    /** @brief Promotes the latest status. Only one reader thread may call this function. */
    void SwapStatusBuffers () noexcept;
    [[nodiscard]] const FRenderFrameStatus &GetStatus () const noexcept;

public:

    /** @brief Executes one bounded POD command on the render thread. */
    void HandleRenderCommand ( const FRenderCommand &InCommand, FRenderFrameStatus &InOutStatus ) noexcept override;

private:

    [[nodiscard]] TExpected<void, ERendererError> CreateResources () noexcept;
    void RenderLatest ( const FRenderFrameRequest &InFrame, FRenderFrameStatus &InOutStatus ) noexcept;
    void ApplyResize ( const RHI::FRHIExtent3D &InExtent, FRenderFrameStatus &InOutStatus ) noexcept;

private:

    TSharedPtr<RHI::IRHIDevice> Device;
    FRenderSnapshotStream *SnapshotStream = nullptr;
    RHI::FRHIExtent3D Extent{ 64U, 64U, 1U };

    TOptional<TSharedRef<RHI::IRHIQueue>> Queue;
    TOptional<TSharedRef<RHI::IRHICommandAllocator>> Allocator;
    TOptional<TSharedRef<RHI::IRHICommandList>> CommandList;
    TOptional<TSharedRef<RHI::IRHITexture>> TargetTexture;
    TOptional<TSharedRef<RHI::IRHITextureView>> TargetView;
    TOptional<TSharedRef<RHI::IRHISemaphore>> Timeline;

    RHI::FRHIRenderingDesc RenderingDesc{};
    UInt64 NextTimelineValue = 0ULL;
    Bool bFirstFrame         = true;

    FRenderExecutor Executor;
};

} // namespace Lumen
