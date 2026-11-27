/**
 * @file RenderFrameBenchmarks.cpp
 * @brief CPU cost of proxy synchronization, deterministic graph planning and one opaque Null frame.
 */

#include <Execution/Renderer.hpp>

#include <Frame/RenderFrameContracts.hpp>

#include <Device/RHIConformanceDeviceFactory.hpp>

#include <Benchmarks/Benchmark.hpp>
#include <Benchmarks/BenchmarkOptimizer.hpp>

#include <Rendering/RenderSnapshotStream.hpp>
#include <Rendering/SceneExtractor.hpp>

#include <RenderGraph/RenderGraphContext.hpp>

#include <Templates/Vector.hpp>

#include <array>

namespace Lumen
{

namespace
{

    constexpr UInt64 BenchmarkWorldId = 71001ULL;
    constexpr FRenderAssetId BenchmarkAsset{ 5ULL };

    [[nodiscard]] FRenderSnapshotCapacities MakeStreamCapacities ( const USize InProxyCount ) noexcept
    {
        return FRenderSnapshotCapacities{ .TransformCapacity = InProxyCount + 1ULL,
                                          .MeshCapacity      = InProxyCount,
                                          .CameraCapacity    = 1ULL,
                                          .LightCapacity     = 0ULL };
    }

    struct FProxyScene final
    {
        TVector<FRenderTransformRecord> Transforms;
        TVector<FRenderMeshRecord> Meshes;

        explicit FProxyScene ( const USize InProxyCount ) noexcept
        {
            Transforms.reserve( InProxyCount + 1ULL );
            Meshes.reserve( InProxyCount );
            Transforms.push_back( { .ProxyId = FRenderProxyId( 1ULL ), .Flags = ERenderTransformFlags::Visible } );
            for ( USize Index = 0ULL; Index < InProxyCount; ++Index )
            {
                const FRenderProxyId Proxy( static_cast<UInt64>( Index + 2ULL ) );
                Transforms.push_back( { .ProxyId = Proxy, .Flags = ERenderTransformFlags::Visible } );
                Meshes.push_back( { .ProxyId         = Proxy,
                                    .MeshAssetId     = BenchmarkAsset,
                                    .MaterialAssetId = FRenderAssetId( 6ULL ),
                                    .TransformIndex  = Index + 1ULL,
                                    .Flags           = ERenderMeshFlags::Visible } );
            }
        }
    };

    /** @brief Steady-state canonical diff of a complete snapshot against the registry. */
    LUMEN_BENCHMARK_ARGS( FRenderFrame, ProxyDiff, 100, 1000, 10000 )
    {
        const USize ProxyCount = static_cast<USize>( State.GetArgument() );
        const FProxyScene Scene( ProxyCount );
        FRenderProxyRegistry Registry( ProxyCount + 1ULL );
        FRenderProxyStats Stats;

        State.SetItemsPerIteration( static_cast<UInt64>( ProxyCount ) );
        for ( auto _ : State )
        {
            const auto Result = Registry.Sync( Scene.Meshes, Scene.Transforms, 1ULL, Stats );
            LUMEN_ASSERT( Result.HasValue() );
            Benchmarks::DoNotOptimize( Registry.GetSize() );
        }
    }

    /** @brief Capacity-bounded declaration and deterministic compilation for N side-effect passes. */
    LUMEN_BENCHMARK_ARGS( FRenderFrame, GraphPlan, 100, 1000, 10000 )
    {
        const USize PassCount = static_cast<USize>( State.GetArgument() );
        FRenderGraphBuilder Builder( FRenderGraphCapacities{
            .PassCount = PassCount, .ResourceCount = PassCount, .WriteCountPerPass = 1ULL, .TransitionCount = 4ULL } );

        State.SetItemsPerIteration( static_cast<UInt64>( PassCount ) );
        for ( auto _ : State )
        {
            Builder.Reset();
            for ( USize Index = 0ULL; Index < PassCount; ++Index )
            {
                const auto Pass = Builder.AddPass( { .bHasSideEffects = true } );
                LUMEN_ASSERT( Pass.HasValue() );
            }
            auto Compiled = std::move( Builder ).Compile();
            LUMEN_ASSERT( Compiled.HasValue() );
            Benchmarks::DoNotOptimize( Compiled->GetExecutionOrder().size() );
        }
    }

    /** @brief Whole CPU frame: snapshot consumed, proxies diffed, graph compiled, indexed draws recorded and submitted. */
    LUMEN_BENCHMARK_ARGS( FRenderFrame_Null, OpaqueFrame, 100, 1000, 10000 )
    {
        const USize ProxyCount = static_cast<USize>( State.GetArgument() );
        auto Device            = RHI::Tests::GetRHIConformanceDeviceFactory().CreateDevice( false );
        LUMEN_ASSERT( Device.HasValue() );

        const FProxyScene Scene( ProxyCount );
        FRenderSnapshotStream Stream( MakeStreamCapacities( ProxyCount ) );
        FSceneExtractor Extractor( Stream );
        const FRenderCameraRecord Camera{ .ProxyId = FRenderProxyId( 1ULL ), .Flags = ERenderCameraFlags::Active };
        const auto Published = Extractor.Publish( { .WorldId    = BenchmarkWorldId,
                                                    .FrameIndex = 1ULL,
                                                    .Transforms = Scene.Transforms,
                                                    .Meshes     = Scene.Meshes,
                                                    .Cameras    = TSpan<const FRenderCameraRecord>( &Camera, 1ULL ) } );
        LUMEN_ASSERT( Published.HasValue() );

        const std::array<Maths::FVec3f, 3U> Positions = { Maths::FVec3f{ 0.0F }, Maths::FVec3f{ 1.0F, 0.0F, 0.0F },
                                                          Maths::FVec3f{ 0.0F, 1.0F, 0.0F } };
        const std::array<UInt32, 3U> Indices          = { 0U, 1U, 2U };

        FRenderer Renderer;
        LUMEN_ASSERT(
            Renderer.RegisterGeometry( { .Id = BenchmarkAsset, .Positions = Positions, .Indices = Indices } ).HasValue() );
        Benchmarks::DoNotOptimize( Positions.size() );
        Benchmarks::DoNotOptimize( Indices.size() );
        const FRendererDesc Desc{ .Device          = *Device,
                                  .SnapshotStream  = &Stream,
                                  .Extent          = { 16U, 16U, 1U },
                                  .CommandCapacity = 64ULL,
                                  .Swapchain       = {},
                                  .Shaders         = FRenderShaderSet{ .VertexBytecode   = { Byte{ 1U } },
                                                                       .PixelBytecode    = { Byte{ 1U } },
                                                                       .VertexEntryPoint = "Main",
                                                                       .PixelEntryPoint  = "Main",
                                                                       .Format           = RHI::ERHIShaderFormat::Null },
                                  .ProxyCapacity   = ProxyCount + 1ULL,
                                  .AssetCapacity   = 1ULL,
                                  .Readback        = {} };
        LUMEN_ASSERT( Renderer.Start( Desc ).HasValue() );

        State.SetItemsPerIteration( static_cast<UInt64>( ProxyCount ) );
        UInt64 FrameIndex = 0ULL;
        for ( auto _ : State )
        {
            ++FrameIndex;
            for ( UInt32 Attempt = 0U; Attempt < 100000U; ++Attempt )
            {
                if ( Renderer.RequestRender( FrameIndex ).HasValue() )
                {
                    break;
                }
                Renderer.SwapStatusBuffers();
            }
            for ( UInt32 Attempt = 0U; Attempt < 100000U; ++Attempt )
            {
                Renderer.SwapStatusBuffers();
                if ( Renderer.GetStatus().LastConsumedFrame >= FrameIndex and Renderer.GetStatus().LastSubmittedTimeline > 0ULL )
                {
                    break;
                }
            }
            Benchmarks::DoNotOptimize( Renderer.GetStatus().DrawCallCount );
        }

        Renderer.RequestShutdown();
        Renderer.Join();
    }

} // namespace

} // namespace Lumen
