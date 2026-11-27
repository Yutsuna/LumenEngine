/**
 * @file RenderGraphBenchmarks.cpp
 * @brief CPU cost of deterministic graph compilation and aliased transient placement; no GPU timing is reported.
 */

#include <RenderGraph/CompiledRenderGraph.hpp>
#include <RenderGraph/RenderGraphBuilder.hpp>

#include <Benchmarks/Benchmark.hpp>
#include <Benchmarks/BenchmarkOptimizer.hpp>

#include <Device/RHIConformanceDeviceFactory.hpp>
#include <RHI/Device/IRHIDevice.hpp>
#include <RHI/Memory/IRHITransientResourceAllocator.hpp>

namespace Lumen
{

namespace
{

    [[nodiscard]] FRenderResourceDesc MakeBuffer ( const UInt64 InSize ) noexcept
    {
        return FRenderResourceDesc{
            .Creation =
                RHI::FRHIBufferDesc{
                    .Size         = InSize,
                    .Usage        = RHI::ERHIResourceUsage::UnorderedAccess,
                    .MemoryClass  = RHI::ERHIMemoryClass::DeviceLocal,
                    .InitialState = RHI::ERHIResourceState::Undefined,
                },
            .Requirements = { .Size = InSize, .Alignment = 16ULL, .PreferredMemoryClass = {} },
        };
    }

    [[nodiscard]] FRenderGraphCapacities MakeCapacities ( const USize InPassCount ) noexcept
    {
        return FRenderGraphCapacities{
            .PassCount              = InPassCount,
            .ResourceCount          = InPassCount,
            .ReadCountPerPass       = 0U,
            .WriteCountPerPass      = 1U,
            .DependencyCountPerPass = InPassCount,
            .TransitionCount        = InPassCount,
            .QueueDependencyCount   = InPassCount,
        };
    }

    /** @brief One active pass writes one transient; lifetimes are disjoint and all requests are alias-compatible. */
    [[nodiscard]] TExpected<FCompiledRenderGraph, FRenderError> CompileIndependentPasses ( const USize InPassCount ) noexcept
    {
        FRenderGraphBuilder Builder( MakeCapacities( InPassCount ) );
        for ( USize Index = 0U; Index < InPassCount; ++Index )
        {
            const auto Resource = Builder.CreateTransient( MakeBuffer( 64ULL ) );
            const auto Pass     = Builder.AddPass( { .bHasSideEffects = true } );
            if ( not Resource or not Pass )
            {
                return TUnexpected<FRenderError>( Resource ? Pass.Error() : Resource.Error() );
            }
            const auto Written = Builder.Write( *Pass, *Resource, RHI::ERHIResourceState::UnorderedAccess );
            if ( not Written )
            {
                return TUnexpected<FRenderError>( Written.Error() );
            }
        }
        return std::move( Builder ).Compile();
    }

    /** @brief Declaration and full compile of N independent passes into pre-reserved arenas. */
    LUMEN_BENCHMARK_ARGS( FRRenderGraph, Compile, 10, 100, 1000 )
    {
        const USize PassCount = static_cast<USize>( State.GetArgument() );
        State.SetItemsPerIteration( 1ULL );
        for ( auto _ : State )
        {
            auto Compiled = CompileIndependentPasses( PassCount );
            LUMEN_ASSERT( Compiled.HasValue() );
            Benchmarks::DoNotOptimize( Compiled->GetExecutionOrder().size() );
        }
    }

    /** @brief Steady-state aliased placement of the compiled batch through the portable RHI allocator contract. */
    LUMEN_BENCHMARK_ARGS( FRRenderGraph, AliasedPlacement, 10, 100, 1000 )
    {
        const USize PassCount = static_cast<USize>( State.GetArgument() );
        auto Compiled         = CompileIndependentPasses( PassCount );
        LUMEN_ASSERT( Compiled.HasValue() );

        const auto Device = RHI::Tests::GetRHIConformanceDeviceFactory().CreateDevice( false );
        LUMEN_ASSERT( Device.HasValue() );
        const auto AllocatorResult = ( *Device )->CreateTransientResourceAllocator();
        LUMEN_ASSERT( AllocatorResult.HasValue() );
        const TSharedRef<RHI::IRHITransientResourceAllocator> &Allocator = *AllocatorResult;

        State.SetItemsPerIteration( PassCount );
        for ( auto _ : State )
        {
            Allocator->BeginBatch();
            const auto Placements = Allocator->Allocate( Compiled->GetTransientBatch() );
            Allocator->EndBatch();
            LUMEN_ASSERT( Placements.HasValue() );
            Benchmarks::DoNotOptimize( Placements->size() );
        }
    }

} // namespace

} // namespace Lumen
