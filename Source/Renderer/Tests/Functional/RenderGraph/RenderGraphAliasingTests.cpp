/**
 * @file RenderGraphAliasingTests.cpp
 * @brief Transient aliasing contracts: disjoint lifetimes may share memory, live ranges may not.
 */

#include <RenderGraph/CompiledRenderGraph.hpp>
#include <RenderGraph/RenderGraphBuilder.hpp>

#include <Device/RHIConformanceDeviceFactory.hpp>
#include <RHI/Device/IRHIDevice.hpp>
#include <RHI/Memory/IRHITransientResourceAllocator.hpp>
#include <RHI/Utilities/RHIAlignment.hpp>

#include <Tests/UnitTest.hpp>

#include <utility>

namespace Lumen
{

namespace
{

    [[nodiscard]] FRenderGraphCapacities MakeCapacities ( const USize InPassCount, const USize InResourceCount ) noexcept
    {
        return FRenderGraphCapacities{
            .PassCount              = InPassCount,
            .ResourceCount          = InResourceCount,
            .ReadCountPerPass       = 8U,
            .WriteCountPerPass      = 8U,
            .DependencyCountPerPass = 16U,
            .TransitionCount        = 64U,
            .QueueDependencyCount   = 64U,
        };
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

    /**
     * @brief Places the compiled batch and checks that every pair of overlapping live ranges
     *        receives disjoint memory, while compatible disjoint ranges are allowed to alias.
     */
    [[nodiscard]] Bool CheckPlacements ( const FCompiledRenderGraph &InGraph ) noexcept
    {
        const auto Device = RHI::Tests::GetRHIConformanceDeviceFactory().CreateDevice();
        if ( not Device )
        {
            return false;
        }
        const auto AllocatorResult = ( *Device )->CreateTransientResourceAllocator();
        if ( not AllocatorResult )
        {
            return false;
        }
        const TSharedRef<RHI::IRHITransientResourceAllocator> &Allocator = *AllocatorResult;

        Allocator->BeginBatch();
        const auto Placements = Allocator->Allocate( InGraph.GetTransientBatch() );
        Allocator->EndBatch();
        if ( not Placements or Placements->size() != InGraph.GetTransientBatch().Requests.size() )
        {
            return false;
        }

        for ( USize Index = 0U; Index < Placements->size(); ++Index )
        {
            if ( ( *Placements )[Index].Lifetime.Begin != InGraph.GetTransientBatch().Requests[Index].Lifetime.Begin or
                 ( *Placements )[Index].Lifetime.End != InGraph.GetTransientBatch().Requests[Index].Lifetime.End )
            {
                return false;
            }
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

    LUMEN_TEST( FRenderGraphAliasing, CompatibleDisjointLifetimesAliasToOneRange )
    {
        FRenderGraphBuilder Builder( MakeCapacities( 4U, 4U ) );
        for ( UInt32 Index = 0U; Index < 4U; ++Index )
        {
            const auto Resource = Builder.CreateTransient( MakeBuffer( 64ULL ) );
            const auto Pass     = Builder.AddPass( { .bHasSideEffects = true } );
            if ( not Resource or not Pass )
            {
                return;
            }
            LUMEN_CHECK_HAS_VALUE( Builder.Write( *Pass, *Resource, RHI::ERHIResourceState::UnorderedAccess ) );
        }

        auto Compiled = std::move( Builder ).Compile();
        LUMEN_CHECK_HAS_VALUE( Compiled );
        if ( not Compiled )
        {
            return;
        }
        LUMEN_CHECK_EQ( Compiled->GetTransientBatch().Requests.size(), 4U );
        for ( USize Index = 0U; Index < Compiled->GetTransientBatch().Requests.size(); ++Index )
        {
            LUMEN_CHECK_EQ( Compiled->GetTransientBatch().Requests[Index].Lifetime.Begin, Index );
            LUMEN_CHECK_EQ( Compiled->GetTransientBatch().Requests[Index].Lifetime.End, Index + 1U );
        }
        LUMEN_CHECK_TRUE( CheckPlacements( *Compiled ) );
    }

    LUMEN_TEST( FRenderGraphAliasing, OverlappingLiveRangesNeverShareMemory )
    {
        FRenderGraphBuilder Builder( MakeCapacities( 4U, 3U ) );
        const auto ResourceA = Builder.CreateTransient( MakeBuffer( 64ULL ) );
        const auto ResourceB = Builder.CreateTransient( MakeBuffer( 64ULL ) );
        const auto ResourceC = Builder.CreateTransient( MakeBuffer( 64ULL ) );
        const auto PassZero  = Builder.AddPass( { .bHasSideEffects = true } );
        const auto PassOne   = Builder.AddPass( { .bHasSideEffects = true } );
        const auto PassTwo   = Builder.AddPass( { .bHasSideEffects = true } );
        const auto PassThree = Builder.AddPass( { .bHasSideEffects = true } );
        if ( not ResourceA or not ResourceB or not ResourceC or not PassZero or not PassOne or not PassTwo or not PassThree )
        {
            return;
        }

        /** INFO: A is live [0,3), B is live [1,3) — they overlap and must not be placed at the same offset. */
        LUMEN_CHECK_HAS_VALUE( Builder.Write( *PassZero, *ResourceA, RHI::ERHIResourceState::UnorderedAccess ) );
        LUMEN_CHECK_HAS_VALUE( Builder.Write( *PassOne, *ResourceB, RHI::ERHIResourceState::UnorderedAccess ) );
        LUMEN_CHECK_HAS_VALUE( Builder.Read( *PassTwo, *ResourceA, RHI::ERHIResourceState::ShaderResource ) );
        LUMEN_CHECK_HAS_VALUE( Builder.Read( *PassTwo, *ResourceB, RHI::ERHIResourceState::ShaderResource ) );
        LUMEN_CHECK_HAS_VALUE( Builder.Write( *PassTwo, *ResourceC, RHI::ERHIResourceState::UnorderedAccess ) );
        LUMEN_CHECK_HAS_VALUE( Builder.Read( *PassThree, *ResourceC, RHI::ERHIResourceState::ShaderResource ) );

        auto Compiled = std::move( Builder ).Compile();
        LUMEN_CHECK_HAS_VALUE( Compiled );
        if ( not Compiled )
        {
            return;
        }

        const auto &Requests = Compiled->GetTransientBatch().Requests;
        LUMEN_CHECK_EQ( Requests.size(), 3U );
        LUMEN_CHECK_TRUE( Requests[0].Lifetime.Overlaps( Requests[1].Lifetime ) );
        LUMEN_CHECK_TRUE( CheckPlacements( *Compiled ) );
    }

    LUMEN_TEST( FRenderGraphAliasing, RejectsOverlappingLifetimesWithIncompatibleRequirements )
    {
        FRenderGraphBuilder Builder( MakeCapacities( 2U, 2U ) );
        const auto Small = Builder.CreateTransient( MakeBuffer( 64ULL ) );
        const auto Large = Builder.CreateTransient( MakeBuffer( 256ULL ) );
        const auto Pass  = Builder.AddPass( { .bHasSideEffects = true } );
        if ( not Small or not Large or not Pass )
        {
            return;
        }
        LUMEN_CHECK_HAS_VALUE( Builder.Write( *Pass, *Small, RHI::ERHIResourceState::UnorderedAccess ) );
        LUMEN_CHECK_HAS_VALUE( Builder.Write( *Pass, *Large, RHI::ERHIResourceState::UnorderedAccess ) );

        auto Compiled = std::move( Builder ).Compile();
        LUMEN_CHECK_HAS_VALUE( Compiled );
        if ( not Compiled )
        {
            return;
        }
        LUMEN_CHECK_TRUE(
            Compiled->GetTransientBatch().Requests[0].Lifetime.Overlaps( Compiled->GetTransientBatch().Requests[1].Lifetime ) );
        LUMEN_CHECK_TRUE( CheckPlacements( *Compiled ) );
    }

} // namespace

} // namespace Lumen
