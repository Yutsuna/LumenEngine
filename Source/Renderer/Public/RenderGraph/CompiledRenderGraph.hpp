/**
 * @file CompiledRenderGraph.hpp
 * @brief Immutable deterministic products of render graph compilation.
 */

#pragma once

#include <RendererExport.hpp>

#include <Memory/Span.hpp>
#include <RenderGraph/RenderGraph.hpp>

namespace Lumen
{

/** @class FCompiledRenderGraph
 * @brief Owns culled passes, transient requests, barriers, and queue submission metadata.
 * @details Execution positions index ExecutionOrder; segment pass ranges are half-open
 *          positions in that same array. No native or backend object is created here.
 */
class RENDERER_EXPORT FCompiledRenderGraph final : public FNonCopyable
{
public:

    FCompiledRenderGraph ( FCompiledRenderGraph && ) noexcept           = default;
    FCompiledRenderGraph &operator=( FCompiledRenderGraph && ) noexcept = default;
    ~FCompiledRenderGraph () noexcept                                   = default;

    [[nodiscard]] TSpan<const FRenderPass> GetPasses () const noexcept;
    [[nodiscard]] TSpan<const FRenderResource> GetResources () const noexcept;
    [[nodiscard]] TSpan<const UInt32> GetExecutionOrder () const noexcept;
    [[nodiscard]] TSpan<const FRenderTransition> GetTransitions () const noexcept;
    [[nodiscard]] TSpan<const FRenderQueueSegment> GetQueueSegments () const noexcept;
    [[nodiscard]] TSpan<const FRenderQueueDependency> GetQueueDependencies () const noexcept;
    [[nodiscard]] const RHI::FRHITransientBatch &GetTransientBatch () const noexcept;
    [[nodiscard]] TSpan<const FRenderResourceHandle> GetTransientRequestMap () const noexcept;

    [[nodiscard]] TExpected<const FRenderPass *, FRenderError> GetPass ( FRenderPassHandle InPass ) const noexcept;
    [[nodiscard]] TExpected<const FRenderResource *, FRenderError>
    GetResource ( FRenderResourceHandle InResource ) const noexcept;
    [[nodiscard]] TExpected<USize, FRenderError> GetTransientRequestIndex ( FRenderResourceHandle InResource ) const noexcept;

private:

    explicit FCompiledRenderGraph ( FRenderGraph &&InGraph ) noexcept;

private:

    TVector<FRenderPass> Passes;
    TVector<FRenderResource> Resources;
    TVector<UInt32> ExecutionOrder;
    TVector<FRenderTransition> Transitions;
    TVector<FRenderQueueSegment> QueueSegments;
    TVector<FRenderQueueDependency> QueueDependencies;
    RHI::FRHITransientBatch TransientBatch;
    TVector<FRenderResourceHandle> TransientRequestMap;

private:

    friend class FRenderGraphCompiler;
};

} // namespace Lumen
