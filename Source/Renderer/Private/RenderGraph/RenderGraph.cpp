/**
 * @file RenderGraph.cpp
 * @brief Implements one-time arena reservation for render graph storage.
 */

#include <RenderGraph/RenderGraph.hpp>

Lumen::FRenderGraph::FRenderGraph ( const FRenderGraphCapacities &InCapacities ) noexcept : Capacities( InCapacities )
{
    Passes.reserve( Capacities.PassCount );
    Resources.reserve( Capacities.ResourceCount );
    AccessNodes.reserve( Capacities.PassCount * ( Capacities.ReadCountPerPass + Capacities.WriteCountPerPass ) );
    PassStates.reserve( Capacities.PassCount );
    ResourceStates.resize( Capacities.ResourceCount );

    TopologicalOrder.reserve( Capacities.PassCount );
    ExecutionOrder.reserve( Capacities.PassCount );
    ReadyHeap.reserve( Capacities.PassCount );
    CullingStack.reserve( Capacities.PassCount );
    PassSegmentIndices.resize( Capacities.PassCount, InvalidRenderQueueSegmentIndex );

    Transitions.reserve( Capacities.TransitionCount );
    QueueSegments.reserve( Capacities.PassCount );
    QueueDependencies.reserve( Capacities.QueueDependencyCount );
    TransientBatch.Requests.reserve( Capacities.ResourceCount );
    TransientRequestMap.reserve( Capacities.ResourceCount );
}

const Lumen::FRenderGraphCapacities &Lumen::FRenderGraph::GetCapacities () const noexcept
{
    return Capacities;
}
