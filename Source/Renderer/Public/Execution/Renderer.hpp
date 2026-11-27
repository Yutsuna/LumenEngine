/**
 * @file Renderer.hpp
 * @brief Backend-neutral renderer facade driving one render executor.
 */

#pragma once

#include <RendererExport.hpp>

#include <Core/NonCopyable.hpp>
#include <Core/NonMovable.hpp>

#include <Execution/RenderExecutor.hpp>
#include <Execution/RenderFrameExtension.hpp>

#include <Frame/RenderAssetRegistry.hpp>
#include <Frame/RenderFrameContracts.hpp>
#include <Frame/RenderProxyRegistry.hpp>
#include <RenderGraph/CompiledRenderGraph.hpp>
#include <RenderGraph/RenderGraphBuilder.hpp>

#include <Memory/SharedPtr.hpp>
#include <Memory/Span.hpp>
#include <Templates/Optional.hpp>
#include <Templates/Vector.hpp>

#include <RHI/Commands/RHIRenderingDesc.hpp>
#include <RHI/Memory/IRHIReadbackAllocator.hpp>
#include <RHI/Presentation/RHIAcquiredImage.hpp>
#include <RHI/Types/RHIExtent.hpp>
#include <Rendering/RenderSnapshotView.hpp>

namespace Lumen
{

namespace RHI
{
    class IRHICommandAllocator;
    class IRHICommandList;
    class IRHIDevice;
    class IRHIPipeline;
    class IRHIPipelineLayout;
    class IRHIQueue;
    class IRHISemaphore;
    class IRHIShader;
    class IRHISwapchain;
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
    GeometryFailed,
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

    /** INFO: An absent swapchain selects the offscreen target; the Null backend supplies one only in tests. */
    TSharedPtr<RHI::IRHISwapchain> Swapchain;
    /** INFO: Empty bytecode keeps the historical clear-only slice; nonempty bytecode enables the opaque pass. */
    FRenderShaderSet Shaders;
    USize ProxyCapacity = 256ULL;
    USize AssetCapacity = 64ULL;
    FRenderFrameReadbackDesc Readback;
    /** INFO: When present, replaces the built-in clear/forward command recording. */
    TSharedPtr<IRenderFrameExtension> FrameExtension;
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

    FRenderer () noexcept;
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

    /**
     * @brief Registers indexed geometry for one opaque asset before Start.
     * @details Control-time operation on the caller thread; the data is copied before the render
     *          thread exists, so no span crosses the thread boundary.
     */
    [[nodiscard]] TExpected<void, ERendererError> RegisterGeometry ( const FRenderGeometryAssetDesc &InDesc ) noexcept;
    /** @brief Returns the last completed readback span; valid until the next consumed frame. */
    [[nodiscard]] TOptional<TSpan<const Byte>> GetReadbackData () const noexcept;

    /** @brief Promotes the latest status. Only one reader thread may call this function. */
    void SwapStatusBuffers () noexcept;
    [[nodiscard]] const FRenderFrameStatus &GetStatus () const noexcept;

public:

    /** @brief Executes one bounded POD command on the render thread. */
    void HandleRenderCommand ( const FRenderCommand &InCommand, FRenderFrameStatus &InOutStatus ) noexcept override;

private:

    struct FPendingGeometry
    {
        FRenderAssetId Id;
        TVector<Maths::FVec3f> Positions;
        TVector<UInt32> Indices;
    };

    [[nodiscard]] TExpected<void, ERendererError> CreateResources () noexcept;
    [[nodiscard]] TExpected<void, ERendererError> CreateForwardResources () noexcept;
    [[nodiscard]] TExpected<void, ERendererError> CreatePresentationResources () noexcept;
    [[nodiscard]] Bool RecreateSwapchainViews ( FRenderFrameStatus *InOutStatus = nullptr ) noexcept;
    void RenderLatest ( const FRenderFrameRequest &InFrame, FRenderFrameStatus &InOutStatus ) noexcept;
    void ApplyResize ( const RHI::FRHIExtent3D &InExtent, FRenderFrameStatus &InOutStatus ) noexcept;
    [[nodiscard]] Bool RecordForwardPass ( const FRenderSnapshotView &InView,
                                           RHI::IRHICommandList &InCommandList,
                                           FRenderFrameStatus &InOutStatus ) noexcept;
    [[nodiscard]] Bool AcquireTarget ( FRenderFrameStatus &InOutStatus ) noexcept;
    void PresentTarget ( FRenderFrameStatus &InOutStatus ) noexcept;
    void UpdateReadback ( FRenderFrameStatus &InOutStatus ) noexcept;

private:

    TSharedPtr<RHI::IRHIDevice> Device;
    FRenderSnapshotStream *SnapshotStream = nullptr;
    RHI::FRHIExtent3D Extent{ 64U, 64U, 1U };
    TSharedPtr<RHI::IRHISwapchain> Swapchain;
    FRenderShaderSet Shaders;
    USize ProxyCapacity = 256ULL;
    USize AssetCapacity = 64ULL;
    FRenderFrameReadbackDesc ReadbackDesc;
    TSharedPtr<IRenderFrameExtension> FrameExtension;

    TVector<FPendingGeometry> PendingGeometry;
    FRenderProxyRegistry Proxies;
    FRenderAssetRegistry Assets;

    TOptional<TSharedRef<RHI::IRHIQueue>> Queue;
    TOptional<TSharedRef<RHI::IRHICommandAllocator>> Allocator;
    TOptional<TSharedRef<RHI::IRHICommandList>> CommandList;
    TOptional<TSharedRef<RHI::IRHITexture>> TargetTexture;
    TOptional<TSharedRef<RHI::IRHITextureView>> TargetView;
    TOptional<TSharedRef<RHI::IRHISemaphore>> Timeline;
    TOptional<TSharedRef<RHI::IRHISemaphore>> RenderComplete;
    TOptional<TSharedRef<RHI::IRHIReadbackAllocator>> ReadbackAllocator;
    TOptional<RHI::FRHIAllocation> ReadbackAllocation;
    TOptional<TSpan<const Byte>> ReadbackData;
    TVector<TSharedRef<RHI::IRHITextureView>> SwapchainViews;

    TSharedPtr<RHI::IRHIPipelineLayout> PipelineLayout;
    TOptional<TSharedRef<RHI::IRHIPipeline>> Pipeline;
    TOptional<TSharedRef<RHI::IRHITexture>> AcquiredTarget;
    TOptional<FRenderGraphBuilder> GraphBuilder;
    TOptional<FCompiledRenderGraph> CompiledGraph;

    RHI::FRHIRenderingDesc RenderingDesc{};
    RHI::FRHIAcquiredImage AcquiredImage;
    Bool bHasAcquiredImage   = false;
    Bool bForwardEnabled     = false;
    Bool bPresentation       = false;
    Bool bReadbackEnabled    = false;
    UInt64 NextTimelineValue = 0ULL;
    Bool bFirstFrame         = true;

    FRenderExecutor Executor;
};

} // namespace Lumen
