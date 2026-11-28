/**
 * @file RenderFrameExtension.hpp
 * @brief Backend-neutral extension point for custom frame recording.
 */
#pragma once

#include <Execution/RenderFrameTypes.hpp>
#include <Memory/SharedPtr.hpp>
#include <RHI/Types/RHIError.hpp>
#include <RHI/Types/RHIExtent.hpp>
#include <Rendering/RenderSnapshotView.hpp>
#include <Templates/Expected.hpp>

namespace Lumen
{

namespace RHI
{
    class IRHICommandList;
    class IRHIDevice;
    class IRHITexture;
    class IRHITextureView;
} // namespace RHI

/**
 * @struct FRenderFrameExtensionContext
 * @brief Borrowed render-thread inputs valid only for the current record call.
 */
struct FRenderFrameExtensionContext
{
    const FRenderSnapshotView &Snapshot;
    RHI::IRHICommandList &CommandList;
    TSharedRef<RHI::IRHITexture> TargetTexture;
    TSharedRef<RHI::IRHITextureView> TargetView;
    RHI::FRHIExtent3D Extent;
    FRenderFrameStatus &Status;
};

/**
 * @class IRenderFrameExtension
 * @brief Replaces the renderer's built-in clear/forward record with custom portable RHI passes.
 */
class IRenderFrameExtension
{
public:

    virtual ~IRenderFrameExtension () noexcept = default;

    [[nodiscard]] virtual RHI::FRHIExpected Initialize ( RHI::IRHIDevice &InDevice,
                                                         const RHI::FRHIExtent3D &InExtent ) noexcept         = 0;
    [[nodiscard]] virtual RHI::FRHIExpected Resize ( const RHI::FRHIExtent3D &InExtent ) noexcept             = 0;
    [[nodiscard]] virtual RHI::FRHIExpected Record ( const FRenderFrameExtensionContext &InContext ) noexcept = 0;

    /** @brief Commits record-side state after the queue accepted the submission. */
    virtual void Commit ( UInt64 InTimelineValue, FRenderFrameStatus &InOutStatus ) noexcept = 0;
};

} // namespace Lumen
