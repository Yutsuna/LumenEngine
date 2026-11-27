/**
 * @file RenderGraph.hpp
 * @brief Owning declaration and compilation storage for one render graph.
 */

#pragma once

#include <RendererExport.hpp>

#include <Core/NonCopyable.hpp>
#include <RHI/Memory/RHITransientBatch.hpp>
#include <RenderGraph/RenderGraphTypes.hpp>

namespace Lumen
{

class FRenderGraphBuilder;
class FRenderGraphCompiler;
class FCompiledRenderGraph;

/** @class FRenderGraph
 * @brief Move-only arenas shared by the builder and the consuming compilation.
 * @details Instances are created and mutated through FRenderGraphBuilder.
 */
class RENDERER_EXPORT FRenderGraph final : public FNonCopyable
{
public:

    explicit FRenderGraph ( const FRenderGraphCapacities &InCapacities ) noexcept;
    FRenderGraph ( FRenderGraph && ) noexcept           = default;
    FRenderGraph &operator=( FRenderGraph && ) noexcept = default;
    ~FRenderGraph () noexcept                           = default;

    [[nodiscard]] const FRenderGraphCapacities &GetCapacities () const noexcept;

private:

    struct FAccessNode
    {
        UInt32 PassOrdinal           = InvalidRenderPassOrdinal;
        UInt32 ResourceIndex         = InvalidRenderResourceIndex;
        UInt32 Previous              = InvalidRenderResourceIndex;
        UInt32 Next                  = InvalidRenderResourceIndex;
        RHI::ERHIResourceState State = RHI::ERHIResourceState::Undefined;
        RHI::ERHIQueueType Queue     = RHI::ERHIQueueType::Graphics;
        Bool bWrite                  = false;
    };

    struct FPassCompilerState
    {
        UInt32 IncomingDependencyCount = 0U;
        Bool bActive                   = false;
    };

    struct FResourceCompilerState
    {
        UInt32 FirstAccess                  = InvalidRenderResourceIndex;
        UInt32 LastAccess                   = InvalidRenderResourceIndex;
        UInt32 LastBarrierPass              = InvalidRenderPassOrdinal;
        RHI::ERHIResourceState BarrierState = RHI::ERHIResourceState::Undefined;
        RHI::ERHIQueueType BarrierQueue     = RHI::ERHIQueueType::Graphics;
        Bool bBarrierInitialized            = false;
    };

private:

    FRenderGraphCapacities Capacities;
    UInt32 NextResourceGeneration = 1U;

    TVector<FRenderPass> Passes;
    TVector<FRenderResource> Resources;
    TVector<FAccessNode> AccessNodes;
    TVector<FPassCompilerState> PassStates;
    TVector<FResourceCompilerState> ResourceStates;

    TVector<UInt32> TopologicalOrder;
    TVector<UInt32> ExecutionOrder;
    TVector<UInt32> ReadyHeap;
    TVector<UInt32> CullingStack;
    TVector<UInt32> PassSegmentIndices;

    TVector<FRenderTransition> Transitions;
    TVector<FRenderQueueSegment> QueueSegments;
    TVector<FRenderQueueDependency> QueueDependencies;
    RHI::FRHITransientBatch TransientBatch;
    TVector<FRenderResourceHandle> TransientRequestMap;

private:

    friend class FRenderGraphBuilder;
    friend class FRenderGraphCompiler;
    friend class FCompiledRenderGraph;
};

} // namespace Lumen
