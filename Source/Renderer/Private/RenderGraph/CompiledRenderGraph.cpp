/**
 * @file CompiledRenderGraph.cpp
 * @brief Implements immutable accessors for a compiled render graph.
 */

#include <RenderGraph/CompiledRenderGraph.hpp>

#include <utility>

Lumen::FCompiledRenderGraph::FCompiledRenderGraph ( FRenderGraph &&InGraph ) noexcept
    : Passes( std::move( InGraph.Passes ) ), Resources( std::move( InGraph.Resources ) ),
      ExecutionOrder( std::move( InGraph.ExecutionOrder ) ), Transitions( std::move( InGraph.Transitions ) ),
      QueueSegments( std::move( InGraph.QueueSegments ) ), QueueDependencies( std::move( InGraph.QueueDependencies ) ),
      TransientBatch( std::move( InGraph.TransientBatch ) ), TransientRequestMap( std::move( InGraph.TransientRequestMap ) )
{
}

Lumen::TSpan<const Lumen::FRenderPass> Lumen::FCompiledRenderGraph::GetPasses () const noexcept
{
    return Passes;
}

Lumen::TSpan<const Lumen::FRenderResource> Lumen::FCompiledRenderGraph::GetResources () const noexcept
{
    return Resources;
}

Lumen::TSpan<const Lumen::UInt32> Lumen::FCompiledRenderGraph::GetExecutionOrder () const noexcept
{
    return ExecutionOrder;
}

Lumen::TSpan<const Lumen::FRenderTransition> Lumen::FCompiledRenderGraph::GetTransitions () const noexcept
{
    return Transitions;
}

Lumen::TSpan<const Lumen::FRenderQueueSegment> Lumen::FCompiledRenderGraph::GetQueueSegments () const noexcept
{
    return QueueSegments;
}

Lumen::TSpan<const Lumen::FRenderQueueDependency> Lumen::FCompiledRenderGraph::GetQueueDependencies () const noexcept
{
    return QueueDependencies;
}

const Lumen::RHI::FRHITransientBatch &Lumen::FCompiledRenderGraph::GetTransientBatch () const noexcept
{
    return TransientBatch;
}

Lumen::TSpan<const Lumen::FRenderResourceHandle> Lumen::FCompiledRenderGraph::GetTransientRequestMap () const noexcept
{
    return TransientRequestMap;
}

Lumen::TExpected<const Lumen::FRenderPass *, Lumen::FRenderError>
Lumen::FCompiledRenderGraph::GetPass ( const FRenderPassHandle InPass ) const noexcept
{
    if ( not InPass.IsValid() or InPass.Ordinal >= Passes.size() )
    {
        return TUnexpected<FRenderError>(
            FRenderError{ .Code = ERenderErrorCode::InvalidPassHandle, .PassOrdinal = InPass.Ordinal } );
    }
    return &Passes[InPass.Ordinal];
}

Lumen::TExpected<const Lumen::FRenderResource *, Lumen::FRenderError>
Lumen::FCompiledRenderGraph::GetResource ( const FRenderResourceHandle InResource ) const noexcept
{
    if ( not InResource.IsValid() or InResource.Index >= Resources.size() or Resources[InResource.Index].Handle != InResource )
    {
        return TUnexpected<FRenderError>(
            FRenderError{ .Code = ERenderErrorCode::InvalidResourceHandle, .ResourceIndex = InResource.Index } );
    }
    return &Resources[InResource.Index];
}

Lumen::TExpected<Lumen::USize, Lumen::FRenderError>
Lumen::FCompiledRenderGraph::GetTransientRequestIndex ( const FRenderResourceHandle InResource ) const noexcept
{
    LUMEN_EXPECT( Resource, GetResource( InResource ) );
    if ( Resource->bImported )
    {
        return TUnexpected<FRenderError>(
            FRenderError{ .Code = ERenderErrorCode::InvalidAccess, .ResourceIndex = InResource.Index } );
    }
    if ( not Resource->bActive )
    {
        return TUnexpected<FRenderError>(
            FRenderError{ .Code = ERenderErrorCode::ResourceCulled, .ResourceIndex = InResource.Index } );
    }
    if ( Resource->TransientRequestIndex >= TransientBatch.Requests.size() )
    {
        return TUnexpected<FRenderError>(
            FRenderError{ .Code = ERenderErrorCode::InvalidTransientBatch, .ResourceIndex = InResource.Index } );
    }
    return static_cast<USize>( Resource->TransientRequestIndex );
}
