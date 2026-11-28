/**
 * @file RenderGraphContext.hpp
 * @brief Execution-time binding and RHI barrier materialization for a compiled graph.
 */

#pragma once

#include <RenderGraph/CompiledRenderGraph.hpp>

namespace Lumen
{

/** @class FRenderGraphContext
 * @brief Owns one compiled graph and binds allocator-produced transients outside draw and dispatch records.
 * @details The context does not create RHI resources, record command lists, or own
 *          IRHITransientResourceAllocator. It moves the compiled plan in, so bindings and
 *          barriers can never outlive their metadata; it only adds binding fronts.
 */
class FRenderGraphContext final
{
public:

    /** @brief Takes ownership of a compiled graph; pass an rvalue so no temporary can dangle. */
    explicit FRenderGraphContext ( FCompiledRenderGraph &&InGraph ) noexcept;

    FRenderGraphContext ( const FRenderGraphContext & ) noexcept           = delete;
    FRenderGraphContext &operator=( const FRenderGraphContext & ) noexcept = delete;

    FRenderGraphContext ( FRenderGraphContext && ) noexcept           = default;
    FRenderGraphContext &operator=( FRenderGraphContext && ) noexcept = default;
    ~FRenderGraphContext () noexcept                                  = default;

public:

    [[nodiscard]] FRenderExpected BindTransient ( FRenderResourceHandle InResource,
                                                  TSharedRef<RHI::IRHIBuffer> InBuffer ) noexcept;
    [[nodiscard]] FRenderExpected BindTransient ( FRenderResourceHandle InResource,
                                                  TSharedRef<RHI::IRHITexture> InTexture ) noexcept;

    [[nodiscard]] TExpected<FRenderResourceBinding, FRenderError>
    GetResourceBinding ( FRenderResourceHandle InResource ) const noexcept;
    [[nodiscard]] TExpected<RHI::FRHITransition, FRenderError> ResolveTransition ( USize InTransitionIndex ) const noexcept;

private:

    FCompiledRenderGraph Graph;
    TVector<FRenderResourceBinding> Bindings;
};

} // namespace Lumen
