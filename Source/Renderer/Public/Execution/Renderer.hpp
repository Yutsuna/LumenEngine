/**
 * @file Renderer.hpp
 * @brief Backend-neutral renderer facade driving one render executor.
 * @details All RHI objects, backend selection and presentation live in Renderer/Private behind the
 *          opaque FRendererState. No device, adapter or swapchain type is exposed here.
 */

#pragma once

#include <RendererExport.hpp>

#include <Core/CoreTypes.hpp>
#include <Core/NonCopyable.hpp>
#include <Core/NonMovable.hpp>

#include <Execution/RenderExecutor.hpp>
#include <Execution/RenderFeature.hpp>
#include <Execution/RenderFrameTypes.hpp>
#include <Execution/RenderSurfaceDesc.hpp>
#include <Frame/RenderFrameContracts.hpp>

#include <Frame/RenderAssetTypes.hpp>
#include <Frame/RenderProxyRegistry.hpp>

#include <Memory/SharedPtr.hpp>
#include <Memory/Span.hpp>
#include <Memory/UniquePtr.hpp>
#include <String/String.hpp>
#include <Templates/Expected.hpp>
#include <Templates/Optional.hpp>
#include <Templates/Vector.hpp>

namespace Lumen
{

class FRenderSnapshotStream;
struct FRendererState;
struct FRendererInternalAccess;

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
 * @brief Composition-root inputs; no backend factory, device or swapchain is accepted.
 */
struct RENDERER_EXPORT FRendererDesc final
{
    FRenderSnapshotStream *SnapshotStream = nullptr;
    FRenderExtent3D Extent{ 64U, 64U, 1U };
    FRenderSurfaceDesc Surface;
    /** INFO: Opaque backend preference ("auto", "opengl", "null"); interpreted only in Private. */
    FString Backend       = "auto";
    USize CommandCapacity = 64ULL;
    FRenderShaderSet Shaders;
    USize ProxyCapacity = 256ULL;
    USize AssetCapacity = 64ULL;
    FRenderFrameReadbackDesc Readback;
    TSharedPtr<IRenderFeature> Feature;
};

/**
 * @class FRenderer
 * @brief Owns the render thread and turns render commands into portable submissions.
 */
class RENDERER_EXPORT FRenderer final : public IRenderCommandHandler, public FNonCopyable, public FNonMovable
{
public:

    FRenderer () noexcept;
    ~FRenderer () noexcept override;

    /** @brief Creates the private backend resources and starts one render thread. */
    [[nodiscard]] TExpected<void, ERendererError> Start ( const FRendererDesc &InDesc ) noexcept;
    /** @brief Enqueues a latest-snapshot render request. */
    [[nodiscard]] TExpected<void, ERendererError> RequestRender ( UInt64 InFrameIndex ) noexcept;
    /** @brief Enqueues a resize applied on the render thread. */
    [[nodiscard]] TExpected<void, ERendererError> RequestResize ( const FRenderExtent3D &InExtent ) noexcept;
    /** @brief Drains queued commands, then stops and joins the render thread. */
    void RequestShutdown () noexcept;
    void Join () noexcept;
    [[nodiscard]] Bool IsRunning () const noexcept;
    /** @brief Adapter name chosen at Start; empty until a backend was created. */
    [[nodiscard]] const FString &GetDeviceName () const noexcept;

    /**
     * @brief Registers indexed geometry for one opaque asset before Start.
     * @details Control-time operation on the caller thread; data is copied before the render thread exists.
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

    friend struct FRendererInternalAccess;

    TUniquePtr<FRendererState> State;
};

} // namespace Lumen
