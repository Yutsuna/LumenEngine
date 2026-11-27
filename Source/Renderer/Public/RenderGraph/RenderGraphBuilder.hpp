/**
 * @file RenderGraphBuilder.hpp
 * @brief Capacity-bounded declaration API for the CPU render graph compiler.
 */

#pragma once

#include <RendererExport.hpp>

#include <RenderGraph/RenderGraph.hpp>

namespace Lumen
{

class FCompiledRenderGraph;

/** @class FRenderGraphBuilder
 * @brief Declares resources, accesses, explicit dependencies, exports, and pass ordinals.
 * @details No container grows beyond FRenderGraphCapacities. Handles are graph-scoped
 *          and compilation consumes the builder.
 */
class RENDERER_EXPORT FRenderGraphBuilder final : public FNonCopyable
{
public:

    explicit FRenderGraphBuilder ( const FRenderGraphCapacities &InCapacities ) noexcept;
    FRenderGraphBuilder ( FRenderGraphBuilder && ) noexcept           = default;
    FRenderGraphBuilder &operator=( FRenderGraphBuilder && ) noexcept = default;
    ~FRenderGraphBuilder () noexcept                                  = default;

    [[nodiscard]] TExpected<FRenderResourceHandle, FRenderError> CreateTransient ( const FRenderResourceDesc &InDesc ) noexcept;

    [[nodiscard]] TExpected<FRenderResourceHandle, FRenderError> Import ( TSharedRef<RHI::IRHIBuffer> InBuffer,
                                                                          const FRenderImportedResourceDesc &InDesc ) noexcept;

    [[nodiscard]] TExpected<FRenderResourceHandle, FRenderError> Import ( TSharedRef<RHI::IRHITexture> InTexture,
                                                                          const FRenderImportedResourceDesc &InDesc ) noexcept;

    [[nodiscard]] TExpected<FRenderPassHandle, FRenderError> AddPass ( const FRenderPassDesc &InDesc ) noexcept;

    [[nodiscard]] FRenderExpected
    Read ( FRenderPassHandle InPass, FRenderResourceHandle InResource, RHI::ERHIResourceState InState ) noexcept;

    [[nodiscard]] FRenderExpected
    Write ( FRenderPassHandle InPass, FRenderResourceHandle InResource, RHI::ERHIResourceState InState ) noexcept;

    /** @brief Adds Before -> After independently of resource hazards; duplicates are idempotent. */
    [[nodiscard]] FRenderExpected AddDependency ( FRenderPassHandle InBefore, FRenderPassHandle InAfter ) noexcept;

    /** @brief Keeps every writer of the resource and its transitive producers during culling. */
    [[nodiscard]] FRenderExpected Export ( FRenderResourceHandle InResource ) noexcept;

    [[nodiscard]] TExpected<FCompiledRenderGraph, FRenderError> Compile () && noexcept;

private:

    [[nodiscard]] FRenderExpected ValidatePass ( FRenderPassHandle InPass ) const noexcept;
    [[nodiscard]] FRenderExpected ValidateResource ( FRenderResourceHandle InResource ) const noexcept;
    [[nodiscard]] FRenderExpected AddAccess ( FRenderPassHandle InPass,
                                              FRenderResourceHandle InResource,
                                              RHI::ERHIResourceState InState,
                                              Bool bWrite ) noexcept;
    [[nodiscard]] TExpected<FRenderResourceHandle, FRenderError> AddResource ( FRenderResource &&InResource ) noexcept;

private:

    FRenderGraph Graph;
    Bool bCompiled = false;
};

} // namespace Lumen
