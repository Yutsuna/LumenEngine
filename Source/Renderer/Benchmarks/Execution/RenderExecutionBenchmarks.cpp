/**
 * @file RenderExecutionBenchmarks.cpp
 * @brief Steady-state enqueue and Null clear-frame CPU costs; no GPU timing is reported.
 */

#include <Backend/StartWithTestDevice.hpp>
#include <Execution/Renderer.hpp>

#include <World/World.hpp>
#include <World/WorldTypes.hpp>

#include <Benchmarks/Benchmark.hpp>
#include <Benchmarks/BenchmarkOptimizer.hpp>

#include <Device/RHIConformanceDeviceFactory.hpp>

#include <Thread/Thread.hpp>

namespace Lumen
{

namespace
{

    /**
     * Private
     */

    /** @brief Allocation-free handler so the measured enqueue path stays representative. */
    class FNoopHandler final : public IRenderCommandHandler
    {
    public:

        void HandleRenderCommand ( const FRenderCommand &InCommand, FRenderFrameStatus &InOutStatus ) noexcept override
        {
            InOutStatus.LastRequestedFrame = InCommand.Frame.FrameIndex;
        }
    };

    [[nodiscard]] FWorldDesc MakeWorldDesc () noexcept
    {
        FWorldDesc Desc;
        Desc.WorkerCount      = 1U;
        Desc.MaxActors        = 16ULL;
        Desc.WorldId          = 88001ULL;
        Desc.RenderCapacities = FRenderSnapshotCapacities{
            .TransformCapacity = 16ULL,
            .MeshCapacity      = 16ULL,
            .CameraCapacity    = 16ULL,
            .LightCapacity     = 16ULL,
        };
        return Desc;
    }

    /** @brief Bounded MPSC enqueue with one consumer. QueueFull is a valid backpressure outcome, not an error. */
    LUMEN_BENCHMARK( FRenderExecutor, Enqueue )
    {
        FNoopHandler Handler;
        FRenderExecutor Executor;
        LUMEN_ASSERT( Executor.Start( FRenderExecutorDesc{ .CommandCapacity = 256ULL }, Handler ).HasValue() );

        State.SetItemsPerIteration( 1ULL );
        UInt64 FrameIndex = 0ULL;

        for ( auto _ : State )
        {
            ++FrameIndex;
            const FRenderFrameRequest Request{ .FrameIndex = FrameIndex, .SnapshotSequence = FrameIndex };
            const auto Result = Executor.Enqueue( FRenderCommand::MakeRender( Request ) );
            Benchmarks::DoNotOptimize( Result );
        }

        Executor.RequestStop();
        Executor.Join();
    }

    /** @brief Complete CPU frame: world extraction, snapshot swap, clear recording and portable submission. */
    LUMEN_BENCHMARK_ARGS( FRenderer_NullSlice, ClearFrame, 16, 64, 256 )
    {
        const UInt32 Extent                               = static_cast<UInt32>( State.GetArgument() );
        RHI::Tests::IRHIConformanceDeviceFactory &Factory = RHI::Tests::GetRHIConformanceDeviceFactory();
        Factory.SetDefaultValidation( false );
        const auto Device = Factory.CreateDevice();
        LUMEN_ASSERT( Device.HasValue() );

        FWorld World( MakeWorldDesc() );
        FRenderer Renderer;
        const FRendererDesc Desc{
            .SnapshotStream  = &World.GetRenderSnapshotStream(),
            .Extent          = { Extent, Extent, 1U },
            .Surface         = {},
            .Backend         = {},
            .CommandCapacity = 64ULL,
            .Shaders         = {},
            .ProxyCapacity   = 256ULL,
            .AssetCapacity   = 64ULL,
            .Readback        = {},
            .Feature         = {},
        };
        LUMEN_ASSERT( StartWithTestDevice( Renderer, *Device, Desc ).HasValue() );

        State.SetItemsPerIteration( 1ULL );
        UInt64 FrameIndex = 0ULL;

        for ( auto _ : State )
        {
            ++FrameIndex;
            static_cast<void>( World.TryTick( 1.0 / 60.0 ) );
            auto Result = Renderer.RequestRender( FrameIndex );
            USize Spins = 0ULL;
            while ( not Result and Result.Error() == ERendererError::QueueFull and Spins < 100000ULL )
            {
                ++Spins;
                FThread::Yield();
                Result = Renderer.RequestRender( FrameIndex );
            }
            Benchmarks::DoNotOptimize( Result );
        }

        Renderer.RequestShutdown();
        Renderer.Join();
    }

} // namespace

} // namespace Lumen
