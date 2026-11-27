/**
 * @file RenderGraphContext.hpp
 * @brief Execution-time binding and RHI barrier materialization for a compiled graph.
 */

#pragma once

#include <RendererExport.hpp>

#include <RenderGraph/CompiledRenderGraph.hpp>

namespace Lumen
{

/** @class FRenderGraphContext
 * @brief Binds allocator-produced transients once, outside draw and dispatch records.
 * @details The context does not allocate memory, create RHI resources, record command
 *          lists, or own IRHITransientResourceAllocator. It only retains binding fronts.
 */
class RENDERER_EXPORT FRenderGraphContext final : public FNonCopyable
{
public:

    explicit FRenderGraphContext ( const FCompiledRenderGraph &InGraph ) noexcept;
    FRenderGraphContext ( FRenderGraphContext && ) noexcept           = default;
    FRenderGraphContext &operator=( FRenderGraphContext && ) noexcept = default;
    ~FRenderGraphContext () noexcept                                  = default;

    [[nodiscard]] FRenderExpected BindTransient ( FRenderResourceHandle InResource,
                                                  TSharedRef<RHI::IRHIBuffer> InBuffer ) noexcept;
    [[nodiscard]] FRenderExpected BindTransient ( FRenderResourceHandle InResource,
                                                  TSharedRef<RHI::IRHITexture> InTexture ) noexcept;

    [[nodiscard]] TExpected<FRenderResourceBinding, FRenderError>
    GetResourceBinding ( FRenderResourceHandle InResource ) const noexcept;
    [[nodiscard]] TExpected<RHI::FRHITransition, FRenderError> ResolveTransition ( USize InTransitionIndex ) const noexcept;

private:

    const FCompiledRenderGraph *Graph;
    TVector<FRenderResourceBinding> Bindings;
};

} // namespace Lumen
