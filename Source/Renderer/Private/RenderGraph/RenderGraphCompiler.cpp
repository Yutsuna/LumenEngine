/**
 * @file RenderGraphCompiler.cpp
 * @brief Implements deterministic DAG, culling, lifetime, barrier, and queue planning.
 */

#include <RenderGraph/RenderGraphCompiler.hpp>

#include <Algo/Sort.hpp>
#include <Templates/Array.hpp>

#include <utility>

namespace
{

using Lumen::UInt32;
using Lumen::USize;

Lumen::TUnexpected<Lumen::FRenderError> MakeCompileError ( const Lumen::ERenderErrorCode InCode,
                                                           const UInt32 InPass        = Lumen::InvalidRenderPassOrdinal,
                                                           const UInt32 InResource    = Lumen::InvalidRenderResourceIndex,
                                                           const UInt32 InRelatedPass = Lumen::InvalidRenderPassOrdinal ) noexcept
{
    return Lumen::TUnexpected<Lumen::FRenderError>( Lumen::FRenderError{
        .Code = InCode, .PassOrdinal = InPass, .ResourceIndex = InResource, .RelatedPassOrdinal = InRelatedPass } );
}

void PushReady ( Lumen::TVector<UInt32> &InHeap, const UInt32 InOrdinal )
{
    InHeap.push_back( InOrdinal );
    USize ChildIndex = InHeap.size() - 1U;
    while ( ChildIndex > 0U )
    {
        const USize ParentIndex = ( ChildIndex - 1U ) / 2U;
        if ( InHeap[ParentIndex] <= InOrdinal )
        {
            break;
        }
        InHeap[ChildIndex] = InHeap[ParentIndex];
        ChildIndex         = ParentIndex;
    }
    InHeap[ChildIndex] = InOrdinal;
}

UInt32 PopReady ( Lumen::TVector<UInt32> &InHeap )
{
    const UInt32 Result = InHeap.front();
    const UInt32 Tail   = InHeap.back();
    InHeap.pop_back();
    if ( InHeap.empty() )
    {
        return Result;
    }

    USize ParentIndex = 0U;
    const USize Count = InHeap.size();
    while ( true )
    {
        const USize LeftChild = ( ParentIndex * 2U ) + 1U;
        if ( LeftChild >= Count )
        {
            break;
        }
        const USize RightChild    = LeftChild + 1U;
        const USize SmallestChild = RightChild < Count and InHeap[RightChild] < InHeap[LeftChild] ? RightChild : LeftChild;
        if ( InHeap[SmallestChild] >= Tail )
        {
            break;
        }
        InHeap[ParentIndex] = InHeap[SmallestChild];
        ParentIndex         = SmallestChild;
    }
    InHeap[ParentIndex] = Tail;
    return Result;
}

} // namespace

Lumen::TExpected<Lumen::FCompiledRenderGraph, Lumen::FRenderError>
Lumen::FRenderGraphCompiler::Compile ( FRenderGraph &&InGraph ) noexcept
{
    FRenderGraph Graph( std::move( InGraph ) );
    LUMEN_TRY_EXPECT_ERROR( BuildResourceDependencies( Graph ) );
    LUMEN_TRY_EXPECT_ERROR( TopologicalSort( Graph, false ) );
    LUMEN_TRY_EXPECT_ERROR( CullPasses( Graph ) );
    LUMEN_TRY_EXPECT_ERROR( TopologicalSort( Graph, true ) );
    LUMEN_TRY_EXPECT_ERROR( BuildLifetimes( Graph ) );
    LUMEN_TRY_EXPECT_ERROR( BuildQueueSegments( Graph ) );
    LUMEN_TRY_EXPECT_ERROR( BuildTransitions( Graph ) );
    return FCompiledRenderGraph( std::move( Graph ) );
}

Lumen::FRenderExpected
Lumen::FRenderGraphCompiler::AddDependency ( FRenderGraph &InGraph, const UInt32 InBefore, const UInt32 InAfter ) noexcept
{
    if ( InBefore == InAfter )
    {
        return MakeCompileError( ERenderErrorCode::DependencyCycle, InBefore, InvalidRenderResourceIndex, InAfter );
    }

    FRenderPass &Before = InGraph.Passes[InBefore];
    FRenderPass &After  = InGraph.Passes[InAfter];
    for ( const UInt32 Dependent : Before.Dependents )
    {
        if ( Dependent == InAfter )
        {
            return {};
        }
    }
    if ( Before.Dependents.size() >= InGraph.Capacities.DependencyCountPerPass or
         After.Dependencies.size() >= InGraph.Capacities.DependencyCountPerPass )
    {
        return MakeCompileError( ERenderErrorCode::CapacityExceeded, InBefore, InvalidRenderResourceIndex, InAfter );
    }
    Before.Dependents.push_back( InAfter );
    After.Dependencies.push_back( InBefore );
    return {};
}

Lumen::FRenderExpected Lumen::FRenderGraphCompiler::BuildResourceDependencies ( FRenderGraph &InGraph ) noexcept
{
    InGraph.AccessNodes.clear();
    for ( FRenderGraph::FResourceCompilerState &State : InGraph.ResourceStates )
    {
        State = FRenderGraph::FResourceCompilerState{};
    }

    for ( FRenderPass &Pass : InGraph.Passes )
    {
        auto ProcessAccess = [&InGraph, &Pass] ( const FRenderResourceAccess &InAccess,
                                                 const Bool bWrite ) noexcept -> FRenderExpected
        {
            const UInt32 ResourceIndex                          = InAccess.Resource.Index;
            FRenderResource &Resource                           = InGraph.Resources[ResourceIndex];
            FRenderGraph::FResourceCompilerState &ResourceState = InGraph.ResourceStates[ResourceIndex];
            const UInt32 PreviousIndex                          = ResourceState.LastAccess;
            const UInt32 OldFirstIndex                          = ResourceState.FirstAccess;

            UInt32 LastWriteIndex = InvalidRenderResourceIndex;
            for ( UInt32 AccessIndex = PreviousIndex; AccessIndex != InvalidRenderResourceIndex;
                  AccessIndex        = InGraph.AccessNodes[AccessIndex].Previous )
            {
                if ( InGraph.AccessNodes[AccessIndex].bWrite )
                {
                    LastWriteIndex = AccessIndex;
                    break;
                }
            }

            if ( bWrite )
            {
                if ( LastWriteIndex != InvalidRenderResourceIndex )
                {
                    LUMEN_TRY_EXPECT_ERROR(
                        AddDependency( InGraph, InGraph.AccessNodes[LastWriteIndex].PassOrdinal, Pass.Ordinal ) );
                }
                UInt32 AccessIndex =
                    LastWriteIndex != InvalidRenderResourceIndex ? InGraph.AccessNodes[LastWriteIndex].Next : OldFirstIndex;
                while ( AccessIndex != InvalidRenderResourceIndex )
                {
                    const FRenderGraph::FAccessNode &Previous = InGraph.AccessNodes[AccessIndex];
                    if ( not Previous.bWrite )
                    {
                        LUMEN_TRY_EXPECT_ERROR( AddDependency( InGraph, Previous.PassOrdinal, Pass.Ordinal ) );
                    }
                    AccessIndex = Previous.Next;
                }
            }
            else if ( LastWriteIndex != InvalidRenderResourceIndex )
            {
                LUMEN_TRY_EXPECT_ERROR( AddDependency( InGraph, InGraph.AccessNodes[LastWriteIndex].PassOrdinal, Pass.Ordinal ) );
            }
            else if ( not Resource.bImported )
            {
                return MakeCompileError( ERenderErrorCode::ReadBeforeWrite, Pass.Ordinal, ResourceIndex );
            }

            if ( PreviousIndex != InvalidRenderResourceIndex )
            {
                const FRenderGraph::FAccessNode &Previous = InGraph.AccessNodes[PreviousIndex];
                if ( Previous.State != InAccess.State or Previous.Queue != Pass.Desc.Queue )
                {
                    LUMEN_TRY_EXPECT_ERROR( AddDependency( InGraph, Previous.PassOrdinal, Pass.Ordinal ) );
                }
            }

            const UInt32 CurrentIndex = static_cast<UInt32>( InGraph.AccessNodes.size() );
            InGraph.AccessNodes.push_back( FRenderGraph::FAccessNode{
                .PassOrdinal   = Pass.Ordinal,
                .ResourceIndex = ResourceIndex,
                .Previous      = PreviousIndex,
                .Next          = InvalidRenderResourceIndex,
                .State         = InAccess.State,
                .Queue         = Pass.Desc.Queue,
                .bWrite        = bWrite,
            } );
            if ( PreviousIndex == InvalidRenderResourceIndex )
            {
                ResourceState.FirstAccess = CurrentIndex;
            }
            else
            {
                InGraph.AccessNodes[PreviousIndex].Next = CurrentIndex;
            }
            ResourceState.LastAccess = CurrentIndex;
            return {};
        };

        for ( const FRenderResourceAccess &Access : Pass.Reads )
        {
            LUMEN_TRY_EXPECT_ERROR( ProcessAccess( Access, false ) );
        }
        for ( const FRenderResourceAccess &Access : Pass.Writes )
        {
            LUMEN_TRY_EXPECT_ERROR( ProcessAccess( Access, true ) );
        }
    }
    return {};
}

Lumen::FRenderExpected Lumen::FRenderGraphCompiler::TopologicalSort ( FRenderGraph &InGraph, const Bool bActiveOnly ) noexcept
{
    TVector<UInt32> &Output = bActiveOnly ? InGraph.ExecutionOrder : InGraph.TopologicalOrder;
    Output.clear();
    InGraph.ReadyHeap.clear();
    USize ExpectedCount = 0U;

    for ( FRenderPass &Pass : InGraph.Passes )
    {
        if ( bActiveOnly and not InGraph.PassStates[Pass.Ordinal].bActive )
        {
            Pass.TopologicalPosition = InvalidRenderPassOrdinal;
            continue;
        }
        ++ExpectedCount;
        InGraph.PassStates[Pass.Ordinal].IncomingDependencyCount = static_cast<UInt32>( Pass.Dependencies.size() );
        Pass.TopologicalPosition                                 = InvalidRenderPassOrdinal;
        if ( Pass.Dependencies.empty() )
        {
            PushReady( InGraph.ReadyHeap, Pass.Ordinal );
        }
    }

    while ( not InGraph.ReadyHeap.empty() )
    {
        const UInt32 Ordinal = PopReady( InGraph.ReadyHeap );
        Output.push_back( Ordinal );
        for ( const UInt32 Dependent : InGraph.Passes[Ordinal].Dependents )
        {
            if ( bActiveOnly and not InGraph.PassStates[Dependent].bActive )
            {
                continue;
            }
            UInt32 &Incoming = InGraph.PassStates[Dependent].IncomingDependencyCount;
            --Incoming;
            if ( Incoming == 0U )
            {
                PushReady( InGraph.ReadyHeap, Dependent );
            }
        }
    }

    if ( Output.size() != ExpectedCount )
    {
        for ( const FRenderPass &Pass : InGraph.Passes )
        {
            if ( ( not bActiveOnly or InGraph.PassStates[Pass.Ordinal].bActive ) and
                 InGraph.PassStates[Pass.Ordinal].IncomingDependencyCount != 0U )
            {
                const UInt32 Related = Pass.Dependencies.empty() ? InvalidRenderPassOrdinal : Pass.Dependencies.front();
                return MakeCompileError( ERenderErrorCode::DependencyCycle, Pass.Ordinal, InvalidRenderResourceIndex, Related );
            }
        }
        return MakeCompileError( ERenderErrorCode::DependencyCycle );
    }

    if ( bActiveOnly )
    {
        for ( USize Position = 0U; Position < Output.size(); ++Position )
        {
            InGraph.Passes[Output[Position]].TopologicalPosition = static_cast<UInt32>( Position );
        }
    }
    return {};
}

Lumen::FRenderExpected Lumen::FRenderGraphCompiler::CullPasses ( FRenderGraph &InGraph ) noexcept
{
    InGraph.CullingStack.clear();
    for ( FRenderPass &Pass : InGraph.Passes )
    {
        InGraph.PassStates[Pass.Ordinal].bActive = false;
        Pass.bActive                             = false;
    }

    auto KeepPass = [&InGraph] ( const UInt32 InOrdinal )
    {
        if ( not InGraph.PassStates[InOrdinal].bActive )
        {
            InGraph.PassStates[InOrdinal].bActive = true;
            InGraph.CullingStack.push_back( InOrdinal );
        }
    };

    for ( const FRenderPass &Pass : InGraph.Passes )
    {
        if ( Pass.Desc.bHasSideEffects )
        {
            KeepPass( Pass.Ordinal );
        }
    }
    for ( const FRenderResource &Resource : InGraph.Resources )
    {
        if ( not Resource.bExported )
        {
            continue;
        }
        Bool bHasWriter = false;
        for ( const FRenderPass &Pass : InGraph.Passes )
        {
            for ( const FRenderResourceAccess &Access : Pass.Writes )
            {
                if ( Access.Resource == Resource.Handle )
                {
                    bHasWriter = true;
                    KeepPass( Pass.Ordinal );
                }
            }
        }
        if ( not Resource.bImported and not bHasWriter )
        {
            return MakeCompileError( ERenderErrorCode::ExportNotProduced, InvalidRenderPassOrdinal, Resource.Handle.Index );
        }
    }

    while ( not InGraph.CullingStack.empty() )
    {
        const UInt32 Ordinal = InGraph.CullingStack.back();
        InGraph.CullingStack.pop_back();
        for ( const UInt32 Dependency : InGraph.Passes[Ordinal].Dependencies )
        {
            KeepPass( Dependency );
        }
    }
    for ( FRenderPass &Pass : InGraph.Passes )
    {
        Pass.bActive = InGraph.PassStates[Pass.Ordinal].bActive;
    }
    return {};
}

Lumen::FRenderExpected Lumen::FRenderGraphCompiler::BuildLifetimes ( FRenderGraph &InGraph ) noexcept
{
    InGraph.TransientBatch.Requests.clear();
    InGraph.TransientRequestMap.clear();
    for ( FRenderResource &Resource : InGraph.Resources )
    {
        Resource.bActive               = false;
        Resource.Lifetime              = RHI::FRHIResourceLifetime{};
        Resource.TransientRequestIndex = InvalidRenderResourceIndex;
    }

    for ( USize Position = 0U; Position < InGraph.ExecutionOrder.size(); ++Position )
    {
        const FRenderPass &Pass = InGraph.Passes[InGraph.ExecutionOrder[Position]];
        auto TouchResource      = [Position] ( FRenderResource &InResource, const FRenderResourceAccess & )
        {
            if ( not InResource.bActive )
            {
                InResource.bActive        = true;
                InResource.Lifetime.Begin = static_cast<UInt64>( Position );
            }
            InResource.Lifetime.End = static_cast<UInt64>( Position + 1U );
        };
        for ( const FRenderResourceAccess &Access : Pass.Reads )
        {
            TouchResource( InGraph.Resources[Access.Resource.Index], Access );
        }
        for ( const FRenderResourceAccess &Access : Pass.Writes )
        {
            TouchResource( InGraph.Resources[Access.Resource.Index], Access );
        }
    }

    for ( FRenderResource &Resource : InGraph.Resources )
    {
        if ( not Resource.bActive )
        {
            if ( Resource.bExported and Resource.bImported and
                 ( Resource.Binding.InitialState != Resource.Binding.FinalState or
                   Resource.Binding.InitialQueue != Resource.Binding.FinalQueue ) )
            {
                return MakeCompileError( ERenderErrorCode::InvalidTransition, InvalidRenderPassOrdinal, Resource.Handle.Index );
            }
            continue;
        }
        if ( Resource.bImported )
        {
            continue;
        }

        Resource.TransientRequestIndex = static_cast<UInt32>( InGraph.TransientBatch.Requests.size() );
        InGraph.TransientBatch.Requests.push_back(
            RHI::FRHITransientDesc{ .Requirements = Resource.Desc.Requirements, .Lifetime = Resource.Lifetime } );
        InGraph.TransientRequestMap.push_back( Resource.Handle );
    }
    if ( not InGraph.TransientBatch.Validate() )
    {
        return MakeCompileError( ERenderErrorCode::InvalidTransientBatch );
    }
    return {};
}

Lumen::FRenderExpected Lumen::FRenderGraphCompiler::AddQueueDependency ( FRenderGraph &InGraph,
                                                                         const UInt32 InSource,
                                                                         const UInt32 InDestination ) noexcept
{
    if ( InSource == InDestination )
    {
        return {};
    }
    for ( const FRenderQueueDependency &Dependency : InGraph.QueueDependencies )
    {
        if ( Dependency.SourceSegmentIndex == InSource and Dependency.DestinationSegmentIndex == InDestination )
        {
            return {};
        }
    }
    if ( InGraph.QueueDependencies.size() >= InGraph.Capacities.QueueDependencyCount )
    {
        return MakeCompileError( ERenderErrorCode::CapacityExceeded, InvalidRenderPassOrdinal, InvalidRenderResourceIndex,
                                 InDestination );
    }
    InGraph.QueueDependencies.push_back( FRenderQueueDependency{
        .SourceSegmentIndex      = InSource,
        .DestinationSegmentIndex = InDestination,
        .WaitValue               = InGraph.QueueSegments[InSource].SignalValue,
    } );
    return {};
}

Lumen::FRenderExpected Lumen::FRenderGraphCompiler::BuildQueueSegments ( FRenderGraph &InGraph ) noexcept
{
    InGraph.QueueSegments.clear();
    InGraph.QueueDependencies.clear();
    TArray<UInt64, 3U> NextSignalValues{ 1ULL, 1ULL, 1ULL };
    TArray<UInt32, 3U> LastSegments{ InvalidRenderQueueSegmentIndex, InvalidRenderQueueSegmentIndex,
                                     InvalidRenderQueueSegmentIndex };

    for ( USize Position = 0U; Position < InGraph.ExecutionOrder.size(); ++Position )
    {
        const UInt32 Ordinal    = InGraph.ExecutionOrder[Position];
        const FRenderPass &Pass = InGraph.Passes[Ordinal];
        const USize QueueIndex  = static_cast<USize>( Pass.Desc.Queue );
        if ( InGraph.QueueSegments.empty() or InGraph.QueueSegments.back().Queue != Pass.Desc.Queue )
        {
            if ( InGraph.QueueSegments.size() >= InGraph.Capacities.PassCount or
                 InGraph.QueueSegments.size() >= InvalidRenderQueueSegmentIndex )
            {
                return MakeCompileError( ERenderErrorCode::CapacityExceeded, Ordinal );
            }
            const UInt32 SegmentIndex = static_cast<UInt32>( InGraph.QueueSegments.size() );
            InGraph.QueueSegments.push_back( FRenderQueueSegment{
                .Queue       = Pass.Desc.Queue,
                .BeginPass   = static_cast<UInt32>( Position ),
                .EndPass     = static_cast<UInt32>( Position + 1U ),
                .SignalValue = NextSignalValues[QueueIndex],
            } );
            ++NextSignalValues[QueueIndex];
            if ( LastSegments[QueueIndex] != InvalidRenderQueueSegmentIndex )
            {
                LUMEN_TRY_EXPECT_ERROR( AddQueueDependency( InGraph, LastSegments[QueueIndex], SegmentIndex ) );
            }
            LastSegments[QueueIndex] = SegmentIndex;
        }
        else
        {
            InGraph.QueueSegments.back().EndPass = static_cast<UInt32>( Position + 1U );
        }
        InGraph.PassSegmentIndices[Ordinal] = static_cast<UInt32>( InGraph.QueueSegments.size() - 1U );
    }

    for ( const UInt32 Ordinal : InGraph.ExecutionOrder )
    {
        const UInt32 Destination = InGraph.PassSegmentIndices[Ordinal];
        for ( const UInt32 Dependency : InGraph.Passes[Ordinal].Dependencies )
        {
            const UInt32 Source = InGraph.PassSegmentIndices[Dependency];
            LUMEN_TRY_EXPECT_ERROR( AddQueueDependency( InGraph, Source, Destination ) );
        }
    }

    Algo::Sort( InGraph.QueueDependencies,
                [] ( const FRenderQueueDependency &InLeft, const FRenderQueueDependency &InRight ) noexcept
    {
        if ( InLeft.DestinationSegmentIndex != InRight.DestinationSegmentIndex )
        {
            return InLeft.DestinationSegmentIndex < InRight.DestinationSegmentIndex;
        }
        return InLeft.SourceSegmentIndex < InRight.SourceSegmentIndex;
    } );
    for ( FRenderQueueSegment &Segment : InGraph.QueueSegments )
    {
        Segment.FirstDependency = 0U;
        Segment.DependencyCount = 0U;
    }
    for ( USize Index = 0U; Index < InGraph.QueueDependencies.size(); ++Index )
    {
        const FRenderQueueDependency &Dependency = InGraph.QueueDependencies[Index];
        FRenderQueueSegment &Segment             = InGraph.QueueSegments[Dependency.DestinationSegmentIndex];
        if ( Segment.DependencyCount == 0U )
        {
            Segment.FirstDependency = static_cast<UInt32>( Index );
        }
        ++Segment.DependencyCount;
    }
    return {};
}

Lumen::FRenderExpected Lumen::FRenderGraphCompiler::EmitTransition ( FRenderGraph &InGraph,
                                                                     FRenderResource &InResource,
                                                                     const RHI::ERHIResourceState InBefore,
                                                                     const RHI::ERHIResourceState InAfter,
                                                                     const RHI::ERHIQueueType InSourceQueue,
                                                                     const RHI::ERHIQueueType InDestinationQueue,
                                                                     const UInt32 InSourcePass,
                                                                     const UInt32 InDestinationPass ) noexcept
{
    const UInt32 ResourceIndex = InResource.Handle.Index;
    if ( InSourceQueue == InDestinationQueue or InBefore != InAfter )
    {
        if ( InAfter == RHI::ERHIResourceState::Undefined or not RHI::IsValidRHIResourceTransition( InBefore, InAfter ) )
        {
            return MakeCompileError( ERenderErrorCode::InvalidTransition, InDestinationPass, ResourceIndex, InSourcePass );
        }
    }
    else
    {
        return MakeCompileError( ERenderErrorCode::UnsupportedQueueTransfer, InDestinationPass, ResourceIndex, InSourcePass );
    }

    const Bool bRecordsBeforeDestination = InDestinationPass != InvalidRenderPassOrdinal and InSourceQueue == InDestinationQueue;
    const UInt32 RecordPass              = bRecordsBeforeDestination ? InDestinationPass : InSourcePass;
    if ( RecordPass == InvalidRenderPassOrdinal or RecordPass >= InGraph.Passes.size() )
    {
        return MakeCompileError( ERenderErrorCode::InvalidInitialQueue, InDestinationPass, ResourceIndex, InSourcePass );
    }
    if ( InGraph.Transitions.size() >= InGraph.Capacities.TransitionCount )
    {
        return MakeCompileError( ERenderErrorCode::CapacityExceeded, RecordPass, ResourceIndex );
    }

    FRenderTransition Transition;
    Transition.Resource               = InResource.Handle;
    Transition.Range                  = InResource.Range;
    Transition.Before                 = InBefore;
    Transition.After                  = InAfter;
    Transition.SourceQueue            = InSourceQueue;
    Transition.DestinationQueue       = InDestinationQueue;
    Transition.SourcePassOrdinal      = InSourcePass;
    Transition.DestinationPassOrdinal = InDestinationPass;
    Transition.RecordPassOrdinal      = RecordPass;
    Transition.RecordPoint =
        bRecordsBeforeDestination ? ERenderTransitionRecordPoint::BeforePass : ERenderTransitionRecordPoint::AfterPass;
    Transition.SegmentIndex = InGraph.PassSegmentIndices[RecordPass];
    if ( Transition.SegmentIndex == InvalidRenderQueueSegmentIndex )
    {
        return MakeCompileError( ERenderErrorCode::InvalidTransition, RecordPass, ResourceIndex );
    }

    if ( InResource.bImported )
    {
        RHI::FRHITransition RHITransition = RHI::FRHIBufferTransition{};
        if ( std::holds_alternative<RHI::FRHIBufferDesc>( InResource.Desc.Creation ) )
        {
            RHI::FRHIBufferTransition BufferTransition;
            BufferTransition.Buffer           = std::get<TSharedPtr<RHI::IRHIBuffer>>( InResource.Binding.Resource );
            BufferTransition.Before           = InBefore;
            BufferTransition.After            = InAfter;
            BufferTransition.SourceQueue      = InSourceQueue;
            BufferTransition.DestinationQueue = InDestinationQueue;
            RHITransition                     = BufferTransition;
        }
        else
        {
            RHI::FRHITextureTransition TextureTransition;
            TextureTransition.Texture          = std::get<TSharedPtr<RHI::IRHITexture>>( InResource.Binding.Resource );
            TextureTransition.Range            = InResource.Range;
            TextureTransition.Before           = InBefore;
            TextureTransition.After            = InAfter;
            TextureTransition.SourceQueue      = InSourceQueue;
            TextureTransition.DestinationQueue = InDestinationQueue;
            RHITransition                      = TextureTransition;
        }
        if ( not RHI::ValidateRHITransition( RHITransition ) )
        {
            return MakeCompileError( ERenderErrorCode::InvalidTransition, RecordPass, ResourceIndex );
        }
        Transition.RHITransition.Emplace( std::move( RHITransition ) );
    }
    InGraph.Transitions.push_back( std::move( Transition ) );
    return {};
}

Lumen::FRenderExpected Lumen::FRenderGraphCompiler::BuildTransitions ( FRenderGraph &InGraph ) noexcept
{
    InGraph.Transitions.clear();
    for ( USize ResourceIndex = 0U; ResourceIndex < InGraph.Resources.size(); ++ResourceIndex )
    {
        const FRenderResource &Resource             = InGraph.Resources[ResourceIndex];
        FRenderGraph::FResourceCompilerState &State = InGraph.ResourceStates[ResourceIndex];
        State.LastBarrierPass                       = InvalidRenderPassOrdinal;
        State.BarrierState        = Resource.bImported ? Resource.Binding.InitialState : RHI::ERHIResourceState::Undefined;
        State.BarrierQueue        = Resource.bImported ? Resource.Binding.InitialQueue : RHI::ERHIQueueType::Graphics;
        State.bBarrierInitialized = Resource.bImported;
    }

    for ( const UInt32 Ordinal : InGraph.ExecutionOrder )
    {
        const FRenderPass &Pass = InGraph.Passes[Ordinal];
        auto ProcessAccess      = [&InGraph, &Pass] ( const FRenderResourceAccess &InAccess ) noexcept -> FRenderExpected
        {
            FRenderResource &Resource                   = InGraph.Resources[InAccess.Resource.Index];
            FRenderGraph::FResourceCompilerState &State = InGraph.ResourceStates[InAccess.Resource.Index];
            const RHI::ERHIResourceState Before =
                State.bBarrierInitialized ? State.BarrierState : RHI::ERHIResourceState::Undefined;
            const RHI::ERHIQueueType SourceQueue = State.bBarrierInitialized ? State.BarrierQueue : Pass.Desc.Queue;
            const UInt32 SourcePass              = State.LastBarrierPass;

            if ( Resource.bImported and SourcePass == InvalidRenderPassOrdinal and SourceQueue != Pass.Desc.Queue )
            {
                return MakeCompileError( ERenderErrorCode::InvalidInitialQueue, Pass.Ordinal, InAccess.Resource.Index );
            }
            if ( Before != InAccess.State or SourceQueue != Pass.Desc.Queue )
            {
                LUMEN_TRY_EXPECT_ERROR( EmitTransition( InGraph, Resource, Before, InAccess.State, SourceQueue, Pass.Desc.Queue,
                                                        SourcePass, Pass.Ordinal ) );
            }
            State.BarrierState        = InAccess.State;
            State.BarrierQueue        = Pass.Desc.Queue;
            State.LastBarrierPass     = Pass.Ordinal;
            State.bBarrierInitialized = true;
            return {};
        };
        for ( const FRenderResourceAccess &Access : Pass.Reads )
        {
            LUMEN_TRY_EXPECT_ERROR( ProcessAccess( Access ) );
        }
        for ( const FRenderResourceAccess &Access : Pass.Writes )
        {
            LUMEN_TRY_EXPECT_ERROR( ProcessAccess( Access ) );
        }
    }

    for ( FRenderResource &Resource : InGraph.Resources )
    {
        if ( not Resource.bImported or not Resource.bActive )
        {
            continue;
        }
        const FRenderGraph::FResourceCompilerState &State = InGraph.ResourceStates[Resource.Handle.Index];
        if ( State.BarrierState != Resource.Binding.FinalState or State.BarrierQueue != Resource.Binding.FinalQueue )
        {
            LUMEN_TRY_EXPECT_ERROR( EmitTransition( InGraph, Resource, State.BarrierState, Resource.Binding.FinalState,
                                                    State.BarrierQueue, Resource.Binding.FinalQueue, State.LastBarrierPass,
                                                    InvalidRenderPassOrdinal ) );
        }
    }

    Algo::Sort( InGraph.Transitions, [&InGraph] ( const FRenderTransition &InLeft, const FRenderTransition &InRight ) noexcept
    {
        if ( InLeft.SegmentIndex != InRight.SegmentIndex )
        {
            return InLeft.SegmentIndex < InRight.SegmentIndex;
        }
        const UInt32 LeftPosition  = InGraph.Passes[InLeft.RecordPassOrdinal].TopologicalPosition;
        const UInt32 RightPosition = InGraph.Passes[InRight.RecordPassOrdinal].TopologicalPosition;
        if ( LeftPosition != RightPosition )
        {
            return LeftPosition < RightPosition;
        }
        if ( InLeft.RecordPoint != InRight.RecordPoint )
        {
            return InLeft.RecordPoint == ERenderTransitionRecordPoint::BeforePass;
        }
        if ( InLeft.Resource.Index != InRight.Resource.Index )
        {
            return InLeft.Resource.Index < InRight.Resource.Index;
        }
        return InLeft.DestinationPassOrdinal < InRight.DestinationPassOrdinal;
    } );
    return {};
}
