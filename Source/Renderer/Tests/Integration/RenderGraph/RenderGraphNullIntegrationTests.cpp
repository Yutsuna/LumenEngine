/**
 * @file RenderGraphNullIntegrationTests.cpp
 * @brief Compiled graph to portable RHI transitions, transients, timelines and release integration on the test-only Null backend.
 */

#include <RenderGraph/CompiledRenderGraph.hpp>
#include <RenderGraph/RenderGraphBuilder.hpp>
#include <RenderGraph/RenderGraphContext.hpp>

#include <Device/NullDevice.hpp>
#include <Device/NullFaultPlan.hpp>
#include <Device/RHIConformanceDeviceFactory.hpp>
#include <RHI/Commands/IRHICommandAllocator.hpp>
#include <RHI/Commands/IRHICommandList.hpp>
#include <RHI/Commands/RHITransition.hpp>
#include <RHI/Device/IRHIDevice.hpp>
#include <RHI/Memory/IRHITransientResourceAllocator.hpp>
#include <RHI/Memory/RHIHeapDesc.hpp>
#include <RHI/Queue/IRHIQueue.hpp>
#include <RHI/Queue/RHISubmitDesc.hpp>
#include <RHI/Resources/IRHIBuffer.hpp>
#include <RHI/Synchronization/IRHISemaphore.hpp>
#include <RHI/Synchronization/RHIDeferredReleaseQueue.hpp>
#include <RHI/Types/RHIError.hpp>

#include <Templates/Function.hpp>
#include <Tests/UnitTest.hpp>

#include <format>
#include <utility>
#include <variant>

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
            .DependencyCountPerPass = 8U,
            .TransitionCount        = 16U,
            .QueueDependencyCount   = 8U,
        };
    }

    [[nodiscard]] FRenderResourceDesc MakeTransientBufferDesc () noexcept
    {
        return FRenderResourceDesc{
            .Creation =
                RHI::FRHIBufferDesc{
                    .Size         = 64ULL,
                    .Usage        = RHI::ERHIResourceUsage::UnorderedAccess,
                    .MemoryClass  = RHI::ERHIMemoryClass::DeviceLocal,
                    .InitialState = RHI::ERHIResourceState::Undefined,
                },
            .Requirements = { .Size = 64ULL, .Alignment = 16ULL, .PreferredMemoryClass = {} },
        };
    }

    /** @brief Records every compiled barrier through the context so each resolved RHI transition is exercised. */
    [[nodiscard]] Bool RecordCompiledTransitions ( const FRenderGraphContext &InContext,
                                                   const FCompiledRenderGraph &InGraph,
                                                   RHI::IRHICommandList &InList ) noexcept
    {
        for ( USize Index = 0U; Index < InGraph.GetTransitions().size(); ++Index )
        {
            const auto Transition = InContext.ResolveTransition( Index );
            if ( not Transition )
            {
                return false;
            }
            const RHI::FRHITransition &Values = *Transition;
            if ( not InList.Transition( TSpan<const RHI::FRHITransition>( &Values, 1U ) ) )
            {
                return false;
            }
        }
        return true;
    }

    LUMEN_INTEGRATION_TEST( FRenderGraph_Null, PlacedTransientSubmitsAndRecyclesAfterTimeline )
    {
        RHI::Tests::IRHIConformanceDeviceFactory &Factory = RHI::Tests::GetRHIConformanceDeviceFactory();
        const auto Device                                 = Factory.CreateDevice( false );
        LUMEN_CHECK_HAS_VALUE( Device );
        if ( not Device )
        {
            return;
        }

        FRenderGraphBuilder Builder( MakeCapacities( 1U, 1U ) );
        const auto Transient = Builder.CreateTransient( MakeTransientBufferDesc() );
        const auto Pass      = Builder.AddPass( { .bHasSideEffects = true } );
        LUMEN_CHECK_HAS_VALUE( Transient );
        LUMEN_CHECK_HAS_VALUE( Pass );
        if ( not Transient or not Pass )
        {
            return;
        }
        LUMEN_CHECK_HAS_VALUE( Builder.Write( *Pass, *Transient, RHI::ERHIResourceState::UnorderedAccess ) );

        auto Compiled = std::move( Builder ).Compile();
        LUMEN_CHECK_HAS_VALUE( Compiled );
        if ( not Compiled )
        {
            return;
        }
        LUMEN_CHECK_EQ( Compiled->GetTransitions().size(), 1U );

        const auto AllocatorResult = ( *Device )->CreateTransientResourceAllocator();
        const auto HeapResult = ( *Device )->CreateHeap( { .Size = 4096ULL, .MemoryClass = RHI::ERHIMemoryClass::DeviceLocal } );
        LUMEN_CHECK_HAS_VALUE( AllocatorResult );
        LUMEN_CHECK_HAS_VALUE( HeapResult );
        if ( not AllocatorResult or not HeapResult )
        {
            return;
        }
        const TSharedRef<RHI::IRHITransientResourceAllocator> &Allocator = *AllocatorResult;
        const TSharedRef<RHI::IRHIHeap> &Heap                            = *HeapResult;
        Allocator->BeginBatch();
        const auto Placements = Allocator->Allocate( Compiled->GetTransientBatch() );
        Allocator->EndBatch();
        LUMEN_CHECK_HAS_VALUE( Placements );
        if ( not Placements or Placements->empty() )
        {
            return;
        }

        RHI::FRHIAllocation Placement = ( *Placements )[0].Allocation;
        Placement.Heap                = Heap.Get();
        const RHI::FRHIBufferDesc Declared =
            std::get<RHI::FRHIBufferDesc>( Compiled->GetResources()[Transient->Index].Desc.Creation );
        const auto PlacedBuffer = ( *Device )->CreatePlacedBuffer( Declared, Heap, Placement );
        LUMEN_CHECK_HAS_VALUE( PlacedBuffer );
        if ( not PlacedBuffer )
        {
            return;
        }

        FRenderGraphContext Context( std::move( *Compiled ) );
        LUMEN_CHECK_HAS_VALUE( Context.BindTransient( *Transient, *PlacedBuffer ) );

        const auto CommandAllocatorResult = ( *Device )->CreateCommandAllocator( RHI::ERHIQueueType::Graphics );
        LUMEN_CHECK_HAS_VALUE( CommandAllocatorResult );
        if ( not CommandAllocatorResult )
        {
            return;
        }
        const TSharedRef<RHI::IRHICommandAllocator> &CommandAllocator = *CommandAllocatorResult;
        const auto CommandListResult                                  = ( *Device )->CreateCommandList( CommandAllocator );
        const auto QueueResult                                        = ( *Device )->GetQueue( RHI::ERHIQueueType::Graphics, 0U );
        const auto TimelineResult                                     = ( *Device )->CreateSemaphore( {} );
        LUMEN_CHECK_HAS_VALUE( CommandListResult );
        LUMEN_CHECK_HAS_VALUE( QueueResult );
        LUMEN_CHECK_HAS_VALUE( TimelineResult );
        if ( not CommandListResult or not QueueResult or not TimelineResult )
        {
            return;
        }
        const TSharedRef<RHI::IRHICommandList> &CommandList = *CommandListResult;
        const TSharedRef<RHI::IRHIQueue> &Queue             = *QueueResult;
        const TSharedRef<RHI::IRHISemaphore> &Timeline      = *TimelineResult;

        LUMEN_CHECK_HAS_VALUE( CommandList->Begin() );
        LUMEN_CHECK_TRUE( RecordCompiledTransitions( Context, *Compiled, *CommandList ) );
        LUMEN_CHECK_HAS_VALUE( CommandList->End() );
        LUMEN_CHECK_EQ( CommandList->GetState(), RHI::ERHICommandListState::Executable );

        RHI::FRHIDeferredReleaseQueue Releases;
        UInt32 ReleasedCount                  = 0U;
        const TFunction<void()> ReleaseBuffer = [&ReleasedCount] () noexcept
        {
            ++ReleasedCount;
        };
        LUMEN_CHECK_HAS_VALUE( Releases.Enqueue( { 6ULL }, ReleaseBuffer ) );

        RHI::FRHISubmitDesc Submit;
        Submit.CommandLists = { CommandList };
        Submit.Signals      = { { .Semaphore = Timeline, .Value = 7ULL } };
        Submit.Completion   = { 5ULL };
        LUMEN_CHECK_HAS_VALUE( Queue->Submit( Submit ) );

        LUMEN_CHECK_EQ( Queue->GetCompletedValue().Value, 5ULL );
        LUMEN_CHECK_EQ( Timeline->GetValue(), 7ULL );
        LUMEN_CHECK_EQ( CommandList->GetState(), RHI::ERHICommandListState::Executable );
        LUMEN_CHECK_EQ( Releases.Poll( Queue->GetCompletedValue() ), 0U );
        LUMEN_CHECK_EQ( ReleasedCount, 0U );
        LUMEN_CHECK_EQ( Releases.Poll( { 6ULL } ), 1U );
        LUMEN_CHECK_EQ( ReleasedCount, 1U );

        LUMEN_CHECK_HAS_VALUE( CommandAllocator->Reset( { 5ULL } ) );
        LUMEN_CHECK_EQ( CommandList->GetState(), RHI::ERHICommandListState::Initial );
    }

    LUMEN_INTEGRATION_TEST( FRenderGraph_Null, ValidatedImportedBarrierIsAccepted )
    {
        const auto Device = RHI::Tests::GetRHIConformanceDeviceFactory().CreateDevice( true );
        LUMEN_CHECK_HAS_VALUE( Device );
        if ( not Device )
        {
            return;
        }
        const auto Buffer = ( *Device )->CreateBuffer( {
            .Size         = 64ULL,
            .Usage        = RHI::ERHIResourceUsage::ShaderResource | RHI::ERHIResourceUsage::UnorderedAccess,
            .InitialState = RHI::ERHIResourceState::Common,
        } );
        LUMEN_CHECK_HAS_VALUE( Buffer );
        if ( not Buffer )
        {
            return;
        }

        FRenderGraphBuilder Builder( MakeCapacities( 1U, 1U ) );
        const auto Imported = Builder.Import( *Buffer, {
                                                           .InitialState = RHI::ERHIResourceState::Common,
                                                           .FinalState   = RHI::ERHIResourceState::ShaderResource,
                                                           .InitialQueue = RHI::ERHIQueueType::Graphics,
                                                           .FinalQueue   = RHI::ERHIQueueType::Graphics,
                                                       } );
        const auto Pass     = Builder.AddPass( { .bHasSideEffects = true } );
        LUMEN_CHECK_HAS_VALUE( Imported );
        LUMEN_CHECK_HAS_VALUE( Pass );
        if ( not Imported or not Pass )
        {
            return;
        }
        LUMEN_CHECK_HAS_VALUE( Builder.Read( *Pass, *Imported, RHI::ERHIResourceState::ShaderResource ) );
        LUMEN_CHECK_HAS_VALUE( Builder.Export( *Imported ) );

        auto Compiled = std::move( Builder ).Compile();
        LUMEN_CHECK_HAS_VALUE( Compiled );
        if ( not Compiled )
        {
            return;
        }
        LUMEN_CHECK_EQ( Compiled->GetTransitions().size(), 1U );

        FRenderGraphContext Context( std::move( *Compiled ) );
        const auto CommandAllocatorResult = ( *Device )->CreateCommandAllocator( RHI::ERHIQueueType::Graphics );
        LUMEN_CHECK_HAS_VALUE( CommandAllocatorResult );
        if ( not CommandAllocatorResult )
        {
            return;
        }
        const TSharedRef<RHI::IRHICommandAllocator> &CommandAllocator = *CommandAllocatorResult;
        const auto CommandListResult                                  = ( *Device )->CreateCommandList( CommandAllocator );
        const auto QueueResult                                        = ( *Device )->GetQueue( RHI::ERHIQueueType::Graphics, 0U );
        LUMEN_CHECK_HAS_VALUE( CommandListResult );
        LUMEN_CHECK_HAS_VALUE( QueueResult );
        if ( not CommandListResult or not QueueResult )
        {
            return;
        }
        const TSharedRef<RHI::IRHICommandList> &CommandList = *CommandListResult;
        const TSharedRef<RHI::IRHIQueue> &Queue             = *QueueResult;

        LUMEN_CHECK_HAS_VALUE( CommandList->Begin() );
        LUMEN_CHECK_TRUE( RecordCompiledTransitions( Context, *Compiled, *CommandList ) );
        LUMEN_CHECK_HAS_VALUE( CommandList->End() );

        RHI::FRHISubmitDesc Submit;
        Submit.CommandLists = { CommandList };
        Submit.Completion   = { 1ULL };
        LUMEN_CHECK_HAS_VALUE( Queue->Submit( Submit ) );
        LUMEN_CHECK_EQ( Queue->GetCompletedValue().Value, 1ULL );
    }

    LUMEN_INTEGRATION_TEST( FRenderGraph_Null, InjectedSubmitFaultLeavesPlanUnsubmitted )
    {
        RHI::Tests::FNullDevice Device;
        Device.GetFaultPlan()->AddFailure(
            "Submit", 1U,
            RHI::FRHIError( RHI::ERHIErrorCode::BackendFailure, RHI::ERHIErrorCondition::Queue, "planned submit fault" ) );

        FRenderGraphBuilder Builder( MakeCapacities( 1U, 0U ) );
        const auto Pass = Builder.AddPass( { .bHasSideEffects = true } );
        LUMEN_CHECK_HAS_VALUE( Pass );
        if ( not Pass )
        {
            return;
        }
        auto Compiled = std::move( Builder ).Compile();
        LUMEN_CHECK_HAS_VALUE( Compiled );
        if ( not Compiled )
        {
            return;
        }
        FRenderGraphContext Context( std::move( *Compiled ) );
        static_cast<void>( Context );

        const auto CommandAllocatorResult = Device.CreateCommandAllocator( RHI::ERHIQueueType::Graphics );
        LUMEN_CHECK_HAS_VALUE( CommandAllocatorResult );
        if ( not CommandAllocatorResult )
        {
            return;
        }
        const TSharedRef<RHI::IRHICommandAllocator> &CommandAllocator = *CommandAllocatorResult;
        const auto CommandListResult                                  = Device.CreateCommandList( CommandAllocator );
        const auto QueueResult                                        = Device.GetQueue( RHI::ERHIQueueType::Graphics, 0U );
        LUMEN_CHECK_HAS_VALUE( CommandListResult );
        LUMEN_CHECK_HAS_VALUE( QueueResult );
        if ( not CommandListResult or not QueueResult )
        {
            return;
        }
        const TSharedRef<RHI::IRHICommandList> &CommandList = *CommandListResult;
        const TSharedRef<RHI::IRHIQueue> &Queue             = *QueueResult;
        LUMEN_CHECK_HAS_VALUE( CommandList->Begin() );
        LUMEN_CHECK_HAS_VALUE( CommandList->End() );

        RHI::FRHISubmitDesc Submit;
        Submit.CommandLists = { CommandList };
        Submit.Completion   = { 1ULL };
        LUMEN_CHECK_HAS_ERROR( Queue->Submit( Submit ) );
        LUMEN_CHECK_EQ( Queue->GetCompletedValue().Value, 0ULL );
        LUMEN_CHECK_EQ( CommandList->GetState(), RHI::ERHICommandListState::Executable );
    }

} // namespace

} // namespace Lumen
