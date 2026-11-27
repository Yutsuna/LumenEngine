/**
 * @file RenderGraphFuzzTests.cpp
 * @brief Literal-seed fuzz against an independent reference model for stable plans and live ranges.
 */

#include <RenderGraph/CompiledRenderGraph.hpp>
#include <RenderGraph/RenderGraphBuilder.hpp>

#include <Device/RHIConformanceDeviceFactory.hpp>
#include <RHI/Device/IRHIDevice.hpp>
#include <RHI/Memory/IRHITransientResourceAllocator.hpp>
#include <RHI/Utilities/RHIAlignment.hpp>

#include <Algo/Equal.hpp>
#include <Tests/UnitTest.hpp>
#include <Utilities/RHITestRandom.hpp>

#include <utility>

namespace Lumen
{

namespace
{

    constexpr USize FuzzPassCount     = 12U;
    constexpr USize FuzzResourceCount = 6U;

    /** @brief Independent stable Kahn model: repeatedly take the smallest ready ordinal. */
    [[nodiscard]] Bool ReferenceOrder ( const TVector<TVector<UInt32>> &InEdges, TVector<UInt32> &OutOrder ) noexcept
    {
        TVector<UInt32> Incoming( FuzzPassCount, 0U );
        for ( const TVector<UInt32> &Targets : InEdges )
        {
            for ( const UInt32 Target : Targets )
            {
                ++Incoming[Target];
            }
        }

        TVector<Bool> Emitted( FuzzPassCount, false );
        OutOrder.clear();
        while ( OutOrder.size() < FuzzPassCount )
        {
            UInt32 Next = InvalidRenderPassOrdinal;
            for ( UInt32 Ordinal = 0U; Ordinal < FuzzPassCount; ++Ordinal )
            {
                if ( not Emitted[Ordinal] and Incoming[Ordinal] == 0U )
                {
                    Next = Ordinal;
                    break;
                }
            }
            if ( Next == InvalidRenderPassOrdinal )
            {
                return false;
            }
            Emitted[Next] = true;
            OutOrder.push_back( Next );
            for ( const UInt32 Target : InEdges[Next] )
            {
                --Incoming[Target];
            }
        }
        return true;
    }

    /** @brief Draws random edges; acyclic mode only points to lower ordinals so cycles cannot occur. */
    void DrawGraph ( RHI::Tests::FRHITestRandom &InRandom,
                     TVector<TVector<UInt32>> &OutEdges,
                     TVector<UInt32> &OutWriters,
                     const Bool bAcyclicOnly ) noexcept
    {
        OutEdges.assign( FuzzPassCount, TVector<UInt32>{} );
        OutWriters.clear();
        for ( USize PassIndex = 0U; PassIndex < FuzzPassCount; ++PassIndex )
        {
            for ( USize TargetIndex = 0U; TargetIndex < FuzzPassCount; ++TargetIndex )
            {
                if ( bAcyclicOnly and TargetIndex < PassIndex )
                {
                    continue;
                }
                if ( PassIndex != TargetIndex and InRandom.Next( 5ULL ) == 0ULL )
                {
                    const UInt32 Target = static_cast<UInt32>( TargetIndex );
                    Bool bDuplicate     = false;
                    for ( const UInt32 Existing : OutEdges[PassIndex] )
                    {
                        bDuplicate = bDuplicate or Existing == Target;
                    }
                    if ( not bDuplicate )
                    {
                        OutEdges[PassIndex].push_back( Target );
                    }
                }
            }
            OutWriters.push_back( static_cast<UInt32>( InRandom.Next( FuzzResourceCount ) ) );
        }
    }

    /** @brief Mirrors the compiler's write-after-write rule so the reference model sees the same DAG. */
    void AddWriterDependencies ( const TVector<UInt32> &InWriters, TVector<TVector<UInt32>> &InOutEdges ) noexcept
    {
        TVector<Int32> LastWriter;
        LastWriter.resize( FuzzResourceCount, -1 );
        for ( USize PassIndex = 0U; PassIndex < InWriters.size(); ++PassIndex )
        {
            const UInt32 Resource = InWriters[PassIndex];
            const Int32 Previous  = LastWriter[Resource];
            if ( Previous >= 0 )
            {
                InOutEdges[static_cast<USize>( Previous )].push_back( static_cast<UInt32>( PassIndex ) );
            }
            LastWriter[Resource] = static_cast<Int32>( PassIndex );
        }
    }

    [[nodiscard]] FRenderResourceDesc MakeBuffer ( const UInt64 InSize ) noexcept
    {
        return FRenderResourceDesc{
            .Creation =
                RHI::FRHIBufferDesc{
                    .Size         = InSize,
                    .Usage        = RHI::ERHIResourceUsage::UnorderedAccess,
                    .MemoryClass  = RHI::ERHIMemoryClass::DeviceLocal,
                    .InitialState = RHI::ERHIResourceState::Common,
                },
            .Requirements = { .Size = InSize, .Alignment = 16ULL, .PreferredMemoryClass = {} },
        };
    }

    [[nodiscard]] FRenderGraphCapacities MakeCapacities () noexcept
    {
        return FRenderGraphCapacities{
            .PassCount              = FuzzPassCount,
            .ResourceCount          = FuzzResourceCount,
            .ReadCountPerPass       = 0U,
            .WriteCountPerPass      = 1U,
            .DependencyCountPerPass = FuzzPassCount,
            .TransitionCount        = FuzzPassCount * 2U,
            .QueueDependencyCount   = FuzzPassCount,
        };
    }

    /**
     * @brief Declares the drawn graph and compiles it. OutExpectedCycle is the reference model's verdict.
     */
    [[nodiscard]] TExpected<FCompiledRenderGraph, FRenderError> CompileDrawnGraph ( const TVector<TVector<UInt32>> &InEdges,
                                                                                    const TVector<UInt32> &InWriters ) noexcept
    {
        FRenderGraphBuilder Builder( MakeCapacities() );
        TVector<FRenderResourceHandle> Resources;
        TVector<FRenderPassHandle> Passes;
        for ( USize Index = 0U; Index < FuzzResourceCount; ++Index )
        {
            auto Resource = Builder.CreateTransient( MakeBuffer( 64ULL ) );
            if ( not Resource )
            {
                return TUnexpected<FRenderError>( Resource.Error() );
            }
            Resources.push_back( *Resource );
        }
        for ( USize Index = 0U; Index < FuzzPassCount; ++Index )
        {
            auto Pass = Builder.AddPass( { .bHasSideEffects = true } );
            if ( not Pass )
            {
                return TUnexpected<FRenderError>( Pass.Error() );
            }
            Passes.push_back( *Pass );
        }
        for ( USize Source = 0U; Source < InEdges.size(); ++Source )
        {
            for ( const UInt32 Target : InEdges[Source] )
            {
                const auto Result = Builder.AddDependency( Passes[Source], Passes[Target] );
                if ( not Result )
                {
                    return TUnexpected<FRenderError>( Result.Error() );
                }
            }
        }
        for ( USize Index = 0U; Index < InWriters.size(); ++Index )
        {
            const auto Result =
                Builder.Write( Passes[Index], Resources[InWriters[Index]], RHI::ERHIResourceState::UnorderedAccess );
            if ( not Result )
            {
                return TUnexpected<FRenderError>( Result.Error() );
            }
        }
        return std::move( Builder ).Compile();
    }

    [[nodiscard]] UInt64 Fingerprint ( const FCompiledRenderGraph &InGraph ) noexcept
    {
        UInt64 Result   = 1469598103934665603ULL;
        const auto Fold = [&Result] ( const UInt64 InValue ) noexcept
        {
            Result ^= InValue;
            Result *= 1099511628211ULL;
        };
        for ( const UInt32 Ordinal : InGraph.GetExecutionOrder() )
        {
            Fold( Ordinal );
        }
        for ( const FRenderResource &Resource : InGraph.GetResources() )
        {
            Fold( Resource.Lifetime.Begin );
            Fold( Resource.Lifetime.End );
        }
        for ( const FRenderTransition &Transition : InGraph.GetTransitions() )
        {
            Fold( static_cast<UInt64>( Transition.After ) );
            Fold( Transition.RecordPassOrdinal );
        }
        return Result;
    }

    /** @brief Checks that all overlapping live ranges received disjoint byte ranges. */
    [[nodiscard]] Bool PlacementsAreDisjoint ( const FCompiledRenderGraph &InGraph ) noexcept
    {
        const auto DeviceResult = RHI::Tests::GetRHIConformanceDeviceFactory().CreateDevice( false );
        if ( not DeviceResult )
        {
            return false;
        }
        const auto AllocatorResult = ( *DeviceResult )->CreateTransientResourceAllocator();
        if ( not AllocatorResult )
        {
            return false;
        }
        const TSharedRef<RHI::IRHITransientResourceAllocator> &Allocator = *AllocatorResult;
        Allocator->BeginBatch();
        const auto Placements = Allocator->Allocate( InGraph.GetTransientBatch() );
        Allocator->EndBatch();
        if ( not Placements )
        {
            return false;
        }
        for ( USize Left = 0U; Left < Placements->size(); ++Left )
        {
            for ( USize Right = Left + 1U; Right < Placements->size(); ++Right )
            {
                if ( ( *Placements )[Left].Lifetime.Overlaps( ( *Placements )[Right].Lifetime ) and
                     RHI::RHIRangesOverlap( ( *Placements )[Left].Allocation.Offset, ( *Placements )[Left].Allocation.Size,
                                            ( *Placements )[Right].Allocation.Offset, ( *Placements )[Right].Allocation.Size ) )
                {
                    return false;
                }
            }
        }
        return true;
    }

    LUMEN_TEST( FRenderGraphFuzz, RandomDagsMatchStableReferenceAndFingerprint )
    {
        constexpr UInt64 Seeds[] = { 0x52475241444801ULL, 0x52475241444802ULL, 0x52475241444803ULL };
        USize CyclicCount        = 0U;
        USize AcyclicCount       = 0U;
        for ( USize SeedIndex = 0U; SeedIndex < 3U; ++SeedIndex )
        {
            const UInt64 Seed = Seeds[SeedIndex];
            RHI::Tests::FRHITestRandom Random( Seed );
            TVector<TVector<UInt32>> Edges;
            TVector<UInt32> Writers;
            DrawGraph( Random, Edges, Writers, SeedIndex % 2U == 0U );
            TVector<TVector<UInt32>> EffectiveEdges = Edges;
            AddWriterDependencies( Writers, EffectiveEdges );
            TVector<UInt32> ExpectedOrder;
            const Bool bAcyclic = ReferenceOrder( EffectiveEdges, ExpectedOrder );

            auto Compiled = CompileDrawnGraph( Edges, Writers );
            if ( not bAcyclic )
            {
                ++CyclicCount;
                LUMEN_CHECK_HAS_ERROR( Compiled );
                if ( Compiled )
                {
                    LUMEN_CHECK_EQ( Compiled.Error().Code, ERenderErrorCode::DependencyCycle );
                }
                continue;
            }

            ++AcyclicCount;
            LUMEN_CHECK_MSG(
                Compiled.HasValue(),
                std::format( "seed={} code={} pass={} resource={}", Seed,
                             static_cast<UInt32>( Compiled ? ERenderErrorCode::CapacityExceeded : Compiled.Error().Code ),
                             Compiled ? 0U : Compiled.Error().PassOrdinal, Compiled ? 0U : Compiled.Error().ResourceIndex ) );
            if ( not Compiled )
            {
                continue;
            }
            LUMEN_CHECK_TRUE( Algo::Equal( Compiled->GetExecutionOrder(), ExpectedOrder ) );

            UInt64 FirstFingerprint = 0ULL;
            for ( UInt32 Run = 0U; Run < 32U; ++Run )
            {
                auto Repeated = CompileDrawnGraph( Edges, Writers );
                LUMEN_CHECK_HAS_VALUE( Repeated );
                if ( not Repeated )
                {
                    break;
                }
                const UInt64 Current = Fingerprint( *Repeated );
                if ( Run == 0U )
                {
                    FirstFingerprint = Current;
                }
                else
                {
                    LUMEN_CHECK_EQ( Current, FirstFingerprint );
                }
            }
        }
        /** INFO: Both reference-model branches must be exercised; otherwise the seeds no longer test cycle handling. */
        LUMEN_CHECK_TRUE( CyclicCount > 0U );
        LUMEN_CHECK_TRUE( AcyclicCount > 0U );
    }

    LUMEN_TEST( FRenderGraphFuzz, RandomLifetimesStayNondegenerateAndAllocationSafe )
    {
        constexpr UInt64 Seeds[] = { 0x52475241414C01ULL, 0x52475241414C02ULL, 0x52475241414C03ULL };
        for ( const UInt64 Seed : Seeds )
        {
            RHI::Tests::FRHITestRandom Random( Seed );
            TVector<TVector<UInt32>> Edges;
            TVector<UInt32> Writers;
            DrawGraph( Random, Edges, Writers, true );
            TVector<TVector<UInt32>> EffectiveEdges = Edges;
            AddWriterDependencies( Writers, EffectiveEdges );
            TVector<UInt32> ExpectedOrder;
            if ( not ReferenceOrder( EffectiveEdges, ExpectedOrder ) )
            {
                continue;
            }

            auto Compiled = CompileDrawnGraph( Edges, Writers );
            LUMEN_CHECK_HAS_VALUE( Compiled );
            if ( not Compiled )
            {
                continue;
            }

            LUMEN_CHECK_TRUE( Compiled->GetTransientBatch().Validate().HasValue() );
            for ( const RHI::FRHITransientDesc &Request : Compiled->GetTransientBatch().Requests )
            {
                LUMEN_CHECK_TRUE( Request.Lifetime.End > Request.Lifetime.Begin );
                LUMEN_CHECK_TRUE( Request.Lifetime.End <= FuzzPassCount );
            }
            /** INFO: A resource written by several passes spans from the first to the last writer position. */
            USize WrittenResourceCount = 0U;
            for ( USize ResourceIndex = 0U; ResourceIndex < FuzzResourceCount; ++ResourceIndex )
            {
                UInt64 FirstWrite = ~0ULL;
                UInt64 LastWrite  = 0ULL;
                for ( USize Position = 0U; Position < ExpectedOrder.size(); ++Position )
                {
                    if ( Writers[ExpectedOrder[Position]] == ResourceIndex )
                    {
                        FirstWrite = FirstWrite == ~0ULL ? Position : FirstWrite;
                        LastWrite  = Position;
                    }
                }
                const FRenderResource &Resource = Compiled->GetResources()[ResourceIndex];
                if ( FirstWrite == ~0ULL )
                {
                    LUMEN_CHECK_FALSE( Resource.bActive );
                    continue;
                }
                ++WrittenResourceCount;
                LUMEN_CHECK_EQ( Resource.Lifetime.Begin, FirstWrite );
                LUMEN_CHECK_EQ( Resource.Lifetime.End, LastWrite + 1U );
            }
            LUMEN_CHECK_EQ( Compiled->GetTransientBatch().Requests.size(), WrittenResourceCount );
            LUMEN_CHECK_TRUE( PlacementsAreDisjoint( *Compiled ) );

            for ( const FRenderQueueSegment &Segment : Compiled->GetQueueSegments() )
            {
                LUMEN_CHECK_TRUE( Segment.SignalValue > 0ULL );
            }
            for ( const FRenderQueueDependency &Dependency : Compiled->GetQueueDependencies() )
            {
                LUMEN_CHECK_TRUE( Dependency.WaitValue > 0ULL );
            }
        }
    }

} // namespace

} // namespace Lumen
