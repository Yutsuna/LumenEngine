/**
 * @file RenderFrameBuilder.hpp
 * @brief Backend-neutral declarative frame graph and pass encoder contracts.
 * @details The graph compiler, barriers and rendering scopes stay in Renderer/Private;
 *          features only declare passes and accesses.
 */

#pragma once

#include <RendererExport.hpp>

#include <Core/CoreTypes.hpp>

#include <Execution/RenderFeature.hpp>
#include <Execution/RenderFrameTypes.hpp>
#include <Memory/UniquePtr.hpp>

namespace Lumen
{

inline constexpr UInt32 InvalidRenderFramePassOrdinal   = 0xFFFFFFFFU;
inline constexpr UInt32 InvalidRenderFrameResourceIndex = 0xFFFFFFFFU;

/**
 * @struct FRenderFramePassHandle
 * @brief Stable declaration ordinal of one pass in a frame graph.
 */
struct FRenderFramePassHandle final
{
    UInt32 Ordinal = InvalidRenderFramePassOrdinal;

    [[nodiscard]] constexpr Bool IsValid () const noexcept
    {
        return Ordinal != InvalidRenderFramePassOrdinal;
    }
};

/**
 * @struct FRenderFrameResourceHandle
 * @brief Graph-scoped index and generation identifying one imported resource.
 */
struct FRenderFrameResourceHandle final
{
    UInt32 Index      = InvalidRenderFrameResourceIndex;
    UInt32 Generation = 0U;

    [[nodiscard]] constexpr Bool IsValid () const noexcept
    {
        return Index != InvalidRenderFrameResourceIndex and Generation != 0U;
    }
};

/**
 * @struct FRenderFrameCapacities
 * @brief Hard capacities reserved once when a frame graph declaration starts.
 */
struct FRenderFrameCapacities final
{
    USize PassCount              = 0U;
    USize ResourceCount          = 0U;
    USize ReadCountPerPass       = 0U;
    USize WriteCountPerPass      = 0U;
    USize DependencyCountPerPass = 0U;
    USize TransitionCount        = 0U;
};

/**
 * @class IRenderPassEncoder
 * @brief Declarative pass commands. Renderer/Private opens and closes the rendering scope
 *        and records the compiled barriers around them.
 */
class RENDERER_EXPORT IRenderPassEncoder
{
public:

    virtual ~IRenderPassEncoder () noexcept = default;

    [[nodiscard]] virtual FRenderExtensionExpected SetPipeline ( const FRenderPipelineRef &InPipeline ) noexcept         = 0;
    [[nodiscard]] virtual FRenderExtensionExpected BindSet ( UInt32 InSlot, const FRenderBindingSetRef &InSet ) noexcept = 0;
    [[nodiscard]] virtual FRenderExtensionExpected
    SetConstants ( ERenderShaderStage InStage, UInt32 InOffset, TSpan<const Byte> InConstants ) noexcept = 0;
    [[nodiscard]] virtual FRenderExtensionExpected Draw ( UInt32 InVertexCount ) noexcept                = 0;
};

/**
 * @class IRenderCompiledFrame
 * @brief Compiled frame graph that records one pass with automatic barrier bookkeeping.
 */
class RENDERER_EXPORT IRenderCompiledFrame
{
public:

    virtual ~IRenderCompiledFrame () noexcept = default;

    /**
     * @brief Records all transitions, the rendering scope, the draw and the trailing transitions
     *        for one declared pass. Committed/Pending texture states are maintained privately.
     */
    [[nodiscard]] virtual FRenderExtensionExpected RecordPass ( IRenderPassEncoder &InEncoder,
                                                                UInt32 InPassOrdinal,
                                                                const FRenderPipelineRef &InPipeline,
                                                                const FRenderBindingSetRef &InBindingSet,
                                                                const FRenderTextureViewRef &InTargetView,
                                                                const FRenderExtent3D &InTargetExtent,
                                                                TSpan<const Byte> InConstants ) noexcept = 0;
};

/**
 * @class IRenderFrameBuilder
 * @brief Declarative replacement for the private RHI render graph builder.
 */
class RENDERER_EXPORT IRenderFrameBuilder
{
public:

    virtual ~IRenderFrameBuilder () noexcept = default;

    virtual void Reset () noexcept = 0;

    /** @brief Imports a texture; the private side reads its committed state itself. */
    [[nodiscard]] virtual TRenderExtensionExpected<FRenderFrameResourceHandle>
    Import ( const FRenderTextureRef &InTexture ) noexcept = 0;

    /** @brief Marks the imported target that ends in the presentable state. */
    [[nodiscard]] virtual FRenderExtensionExpected SetTarget ( const FRenderTextureRef &InTexture ) noexcept = 0;

    [[nodiscard]] virtual TRenderExtensionExpected<FRenderFramePassHandle> AddPass ( Bool bHasSideEffects = false ) noexcept = 0;

    [[nodiscard]] virtual FRenderExtensionExpected Read ( const FRenderFramePassHandle InPass,
                                                          const FRenderFrameResourceHandle InResource ) noexcept  = 0;
    [[nodiscard]] virtual FRenderExtensionExpected Write ( const FRenderFramePassHandle InPass,
                                                           const FRenderFrameResourceHandle InResource ) noexcept = 0;

    [[nodiscard]] virtual TUniquePtr<IRenderCompiledFrame> Compile () noexcept = 0;
};

} // namespace Lumen
