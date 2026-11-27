/**
 * @file RenderGraphCompilerTests.cpp
 * @brief Deterministic DAG, culling, lifetime, barrier, and queue contracts of graph compilation.
 */

#include <RenderGraph/RenderGraphBuilder.hpp>
#include <RenderGraph/RenderGraphContext.hpp>

#include <Device/RHIConformanceDeviceFactory.hpp>
#include <RHI/Device/IRHIDevice.hpp>

#include <Tests/UnitTest.hpp>

#include <utility>

namespace Lumen
{

namespace
{

    /** @brief Generous capacities so each test isolates one contract rather than saturation. */
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
                    .Usage        = RHI::ERHIResourceUsage::ShaderResource | RHI::ERHIResourceUsage::UnorderedAccess,
                    .MemoryClass  = RHI::ERHIMemoryClass::DeviceLocal,
                    .InitialState = RHI::ERHIResourceState::Common,
                },
            .Requirements = { .Size = InSize, .Alignment = 16ULL, .PreferredMemoryClass = {} },
        };
    }

    [[nodiscard]] Bool IsActive ( const FCompiledRenderGraph &InGraph, const FRenderPassHandle InPass ) noexcept
    {
        return ( *InGraph.GetPass( InPass ) )->bActive;
    }

    /** @brief Compiles the same declaration repeatedly and folds the entire plan into one stable hash. */
    [[nodiscard]] UInt64 Fingerprint ()
    {
        FRenderGraphBuilder Builder( MakeCapacities( 4U, 3U ) );
        const auto ResourceA = Builder.CreateTransient( MakeBuffer( 64ULL ) );
        const auto ResourceB = Builder.CreateTransient( MakeBuffer( 128ULL ) );
        const auto ResourceC = Builder.CreateTransient( MakeBuffer( 256ULL ) );
        const auto PassZero  = Builder.AddPass( {} );
        const auto PassOne   = Builder.AddPass( {} );
        const auto PassTwo   = Builder.AddPass( {} );
        const auto PassThree = Builder.AddPass( {} );

        LUMEN_CHECK_HAS_VALUE( ResourceA );
        LUMEN_CHECK_HAS_VALUE( ResourceB );
        LUMEN_CHECK_HAS_VALUE( ResourceC );
        LUMEN_CHECK_HAS_VALUE( PassZero );
        LUMEN_CHECK_HAS_VALUE( PassOne );
        LUMEN_CHECK_HAS_VALUE( PassTwo );
        LUMEN_CHECK_HAS_VALUE( PassThree );

        if ( not ResourceA or not ResourceB or not ResourceC or not PassZero or not PassOne or not PassTwo or not PassThree )
        {
            return 0ULL;
        }

        LUMEN_CHECK_HAS_VALUE( Builder.Write( *PassZero, *ResourceA, RHI::ERHIResourceState::RenderTarget ) );
        LUMEN_CHECK_HAS_VALUE( Builder.Read( *PassOne, *ResourceA, RHI::ERHIResourceState::ShaderResource ) );
        LUMEN_CHECK_HAS_VALUE( Builder.Write( *PassOne, *ResourceB, RHI::ERHIResourceState::RenderTarget ) );
        LUMEN_CHECK_HAS_VALUE( Builder.Read( *PassTwo, *ResourceB, RHI::ERHIResourceState::ShaderResource ) );
        LUMEN_CHECK_HAS_VALUE( Builder.Write( *PassTwo, *ResourceC, RHI::ERHIResourceState::RenderTarget ) );
        LUMEN_CHECK_HAS_VALUE( Builder.Read( *PassThree, *ResourceC, RHI::ERHIResourceState::ShaderResource ) );
        LUMEN_CHECK_HAS_VALUE( Builder.Export( *ResourceC ) );

        auto Compiled = std::move( Builder ).Compile();
        LUMEN_CHECK_HAS_VALUE( Compiled );
        if ( not Compiled )
        {
            return 0ULL;
        }

        UInt64 Result   = 1469598103934665603ULL;
        const auto Fold = [&Result] ( const UInt64 InValue ) noexcept
        {
            Result ^= InValue;
            Result *= 1099511628211ULL;
        };
        for ( const UInt32 Ordinal : Compiled->GetExecutionOrder() )
        {
            Fold( Ordinal );
        }
        for ( const FRenderResource &Resource : Compiled->GetResources() )
        {
            Fold( Resource.Lifetime.Begin );
            Fold( Resource.Lifetime.End );
            Fold( Resource.bActive ? 1ULL : 0ULL );
            Fold( Resource.TransientRequestIndex );
        }
        for ( const FRenderTransition &Transition : Compiled->GetTransitions() )
        {
            Fold( static_cast<UInt64>( Transition.Before ) );
            Fold( static_cast<UInt64>( Transition.After ) );
            Fold( static_cast<UInt64>( Transition.SourceQueue ) );
            Fold( static_cast<UInt64>( Transition.DestinationQueue ) );
            Fold( Transition.RecordPassOrdinal );
            Fold( static_cast<UInt64>( Transition.RecordPoint ) );
        }
        for ( const FRenderQueueSegment &Segment : Compiled->GetQueueSegments() )
        {
            Fold( Segment.BeginPass );
            Fold( Segment.EndPass );
            Fold( Segment.SignalValue );
        }
        for ( const FRenderQueueDependency &Dependency : Compiled->GetQueueDependencies() )
        {
            Fold( Dependency.SourceSegmentIndex );
            Fold( Dependency.DestinationSegmentIndex );
            Fold( Dependency.WaitValue );
        }
        return Result;
    }

    LUMEN_TEST( FRenderGraphCompiler, BuildsStableDagAndHalfOpenLifetimes )
    {
        FRenderGraphBuilder Builder( MakeCapacities( 4U, 3U ) );
        const auto ResourceA = Builder.CreateTransient( MakeBuffer( 64ULL ) );
        const auto ResourceB = Builder.CreateTransient( MakeBuffer( 128ULL ) );
        const auto ResourceC = Builder.CreateTransient( MakeBuffer( 256ULL ) );
        const auto PassZero  = Builder.AddPass( {} );
        const auto PassOne   = Builder.AddPass( {} );
        const auto PassTwo   = Builder.AddPass( {} );
        const auto PassThree = Builder.AddPass( { .bHasSideEffects = true } );
        LUMEN_CHECK_HAS_VALUE( ResourceA );
        LUMEN_CHECK_HAS_VALUE( ResourceB );
        LUMEN_CHECK_HAS_VALUE( ResourceC );
        LUMEN_CHECK_HAS_VALUE( PassZero );
        LUMEN_CHECK_HAS_VALUE( PassOne );
        LUMEN_CHECK_HAS_VALUE( PassTwo );
        LUMEN_CHECK_HAS_VALUE( PassThree );
        if ( not ResourceA or not ResourceB or not ResourceC or not PassZero or not PassOne or not PassTwo or not PassThree )
        {
            return;
        }

        LUMEN_CHECK_HAS_VALUE( Builder.Write( *PassZero, *ResourceA, RHI::ERHIResourceState::RenderTarget ) );
        LUMEN_CHECK_HAS_VALUE( Builder.Read( *PassOne, *ResourceA, RHI::ERHIResourceState::ShaderResource ) );
        LUMEN_CHECK_HAS_VALUE( Builder.Write( *PassOne, *ResourceB, RHI::ERHIResourceState::RenderTarget ) );
        LUMEN_CHECK_HAS_VALUE( Builder.Read( *PassTwo, *ResourceB, RHI::ERHIResourceState::ShaderResource ) );
        LUMEN_CHECK_HAS_VALUE( Builder.Write( *PassTwo, *ResourceC, RHI::ERHIResourceState::RenderTarget ) );
        LUMEN_CHECK_HAS_VALUE( Builder.Read( *PassThree, *ResourceC, RHI::ERHIResourceState::ShaderResource ) );
        LUMEN_CHECK_HAS_VALUE( Builder.Export( *ResourceC ) );

        auto Compiled = std::move( Builder ).Compile();
        LUMEN_CHECK_HAS_VALUE( Compiled );
        if ( not Compiled )
        {
            return;
        }

        LUMEN_CHECK_EQ( Compiled->GetExecutionOrder().size(), 4U );
        LUMEN_CHECK_EQ( Compiled->GetExecutionOrder()[0], PassZero->Ordinal );
        LUMEN_CHECK_EQ( Compiled->GetExecutionOrder()[1], PassOne->Ordinal );
        LUMEN_CHECK_EQ( Compiled->GetExecutionOrder()[2], PassTwo->Ordinal );
        LUMEN_CHECK_EQ( Compiled->GetExecutionOrder()[3], PassThree->Ordinal );

        LUMEN_CHECK_EQ( ( *Compiled->GetResource( *ResourceA ) )->Lifetime.Begin, 0ULL );
        LUMEN_CHECK_EQ( ( *Compiled->GetResource( *ResourceA ) )->Lifetime.End, 2ULL );
        LUMEN_CHECK_EQ( ( *Compiled->GetResource( *ResourceB ) )->Lifetime.Begin, 1ULL );
        LUMEN_CHECK_EQ( ( *Compiled->GetResource( *ResourceB ) )->Lifetime.End, 3ULL );
        LUMEN_CHECK_EQ( ( *Compiled->GetResource( *ResourceC ) )->Lifetime.Begin, 2ULL );
        LUMEN_CHECK_EQ( ( *Compiled->GetResource( *ResourceC ) )->Lifetime.End, 4ULL );

        LUMEN_CHECK_EQ( Compiled->GetTransientBatch().Requests.size(), 3U );
        LUMEN_CHECK_EQ( Compiled->GetTransientBatch().Requests[0].Lifetime.Begin, 0ULL );
        LUMEN_CHECK_EQ( Compiled->GetTransientBatch().Requests[2].Lifetime.End, 4ULL );
        LUMEN_CHECK_HAS_VALUE( Compiled->GetTransientBatch().Validate() );

        const auto IndexA = Compiled->GetTransientRequestIndex( *ResourceA );
        const auto IndexB = Compiled->GetTransientRequestIndex( *ResourceB );
        const auto IndexC = Compiled->GetTransientRequestIndex( *ResourceC );
        LUMEN_CHECK_HAS_VALUE( IndexA );
        LUMEN_CHECK_HAS_VALUE( IndexB );
        LUMEN_CHECK_HAS_VALUE( IndexC );
        if ( IndexA and IndexB and IndexC )
        {
            LUMEN_CHECK_EQ( Compiled->GetTransientRequestMap()[*IndexA], *ResourceA );
            LUMEN_CHECK_EQ( Compiled->GetTransientRequestMap()[*IndexB], *ResourceB );
            LUMEN_CHECK_EQ( Compiled->GetTransientRequestMap()[*IndexC], *ResourceC );
        }

        /** INFO: Four state changes along the chain plus the three Undefined -> write barriers. */
        LUMEN_CHECK_EQ( Compiled->GetTransitions().size(), 6U );
    }

    LUMEN_TEST( FRenderGraphCompiler, CullsConsumersAndKeepsExportedProducers )
    {
        FRenderGraphBuilder Builder( MakeCapacities( 4U, 2U ) );
        const auto ResourceA = Builder.CreateTransient( MakeBuffer( 64ULL ) );
        const auto ResourceB = Builder.CreateTransient( MakeBuffer( 128ULL ) );
        const auto PassZero  = Builder.AddPass( {} );
        const auto PassOne   = Builder.AddPass( {} );
        const auto PassTwo   = Builder.AddPass( {} );
        const auto PassThree = Builder.AddPass( { .bHasSideEffects = true } );
        LUMEN_CHECK_HAS_VALUE( ResourceA );
        LUMEN_CHECK_HAS_VALUE( ResourceB );
        LUMEN_CHECK_HAS_VALUE( PassZero );
        LUMEN_CHECK_HAS_VALUE( PassOne );
        LUMEN_CHECK_HAS_VALUE( PassTwo );
        LUMEN_CHECK_HAS_VALUE( PassThree );
        if ( not ResourceA or not ResourceB or not PassZero or not PassOne or not PassTwo or not PassThree )
        {
            return;
        }

        LUMEN_CHECK_HAS_VALUE( Builder.Write( *PassZero, *ResourceA, RHI::ERHIResourceState::RenderTarget ) );
        LUMEN_CHECK_HAS_VALUE( Builder.Read( *PassOne, *ResourceA, RHI::ERHIResourceState::ShaderResource ) );
        LUMEN_CHECK_HAS_VALUE( Builder.Write( *PassOne, *ResourceB, RHI::ERHIResourceState::RenderTarget ) );
        LUMEN_CHECK_HAS_VALUE( Builder.Read( *PassTwo, *ResourceB, RHI::ERHIResourceState::ShaderResource ) );

        {
            auto Compiled = std::move( Builder ).Compile();
            LUMEN_CHECK_HAS_VALUE( Compiled );
            if ( Compiled )
            {
                LUMEN_CHECK_EQ( Compiled->GetExecutionOrder().size(), 1U );
                LUMEN_CHECK_EQ( Compiled->GetExecutionOrder()[0], PassThree->Ordinal );
                LUMEN_CHECK_FALSE( IsActive( *Compiled, *PassZero ) );
                LUMEN_CHECK_FALSE( ( *Compiled->GetResource( *ResourceA ) )->bActive );
                LUMEN_CHECK_TRUE( Compiled->GetTransientBatch().Requests.empty() );
            }
        }

        FRenderGraphBuilder ExportBuilder( MakeCapacities( 4U, 2U ) );
        const auto ExportA  = ExportBuilder.CreateTransient( MakeBuffer( 64ULL ) );
        const auto ExportB  = ExportBuilder.CreateTransient( MakeBuffer( 128ULL ) );
        const auto ExportP0 = ExportBuilder.AddPass( {} );
        const auto ExportP1 = ExportBuilder.AddPass( {} );
        const auto ExportP2 = ExportBuilder.AddPass( {} );
        const auto ExportP3 = ExportBuilder.AddPass( { .bHasSideEffects = true } );
        LUMEN_CHECK_HAS_VALUE( ExportA );
        LUMEN_CHECK_HAS_VALUE( ExportB );
        LUMEN_CHECK_HAS_VALUE( ExportP0 );
        LUMEN_CHECK_HAS_VALUE( ExportP1 );
        LUMEN_CHECK_HAS_VALUE( ExportP2 );
        LUMEN_CHECK_HAS_VALUE( ExportP3 );
        if ( not ExportA or not ExportB or not ExportP0 or not ExportP1 or not ExportP2 or not ExportP3 )
        {
            return;
        }
        LUMEN_CHECK_HAS_VALUE( ExportBuilder.Write( *ExportP0, *ExportA, RHI::ERHIResourceState::RenderTarget ) );
        LUMEN_CHECK_HAS_VALUE( ExportBuilder.Read( *ExportP1, *ExportA, RHI::ERHIResourceState::ShaderResource ) );
        LUMEN_CHECK_HAS_VALUE( ExportBuilder.Write( *ExportP1, *ExportB, RHI::ERHIResourceState::RenderTarget ) );
        LUMEN_CHECK_HAS_VALUE( ExportBuilder.Read( *ExportP2, *ExportB, RHI::ERHIResourceState::ShaderResource ) );
        LUMEN_CHECK_HAS_VALUE( ExportBuilder.Export( *ExportB ) );

        auto Exported = std::move( ExportBuilder ).Compile();
        LUMEN_CHECK_HAS_VALUE( Exported );
        if ( Exported )
        {
            /** INFO: Export keeps producers P0/P1 and side-effect pass P3, but not the consumer P2. */
            LUMEN_CHECK_EQ( Exported->GetExecutionOrder().size(), 3U );
            LUMEN_CHECK_TRUE( IsActive( *Exported, *ExportP0 ) );
            LUMEN_CHECK_TRUE( IsActive( *Exported, *ExportP1 ) );
            LUMEN_CHECK_FALSE( IsActive( *Exported, *ExportP2 ) );
            LUMEN_CHECK_TRUE( IsActive( *Exported, *ExportP3 ) );
            LUMEN_CHECK_TRUE( ( *Exported->GetResource( *ExportA ) )->bActive );
            LUMEN_CHECK_TRUE( ( *Exported->GetResource( *ExportB ) )->bActive );
        }
    }

    LUMEN_TEST( FRenderGraphCompiler, RejectsDependencyCycles )
    {
        FRenderGraphBuilder Builder( MakeCapacities( 2U, 1U ) );
        const auto PassZero = Builder.AddPass( {} );
        const auto PassOne  = Builder.AddPass( {} );
        LUMEN_CHECK_HAS_VALUE( PassZero );
        LUMEN_CHECK_HAS_VALUE( PassOne );
        if ( not PassZero or not PassOne )
        {
            return;
        }

        LUMEN_CHECK_HAS_ERROR( Builder.AddDependency( *PassZero, *PassZero ) );
        LUMEN_CHECK_HAS_VALUE( Builder.AddDependency( *PassZero, *PassOne ) );
        LUMEN_CHECK_HAS_VALUE( Builder.AddDependency( *PassZero, *PassOne ) );
        LUMEN_CHECK_HAS_VALUE( Builder.AddDependency( *PassOne, *PassZero ) );

        auto Compiled = std::move( Builder ).Compile();
        LUMEN_CHECK_HAS_ERROR( Compiled );
        if ( Compiled )
        {
            return;
        }
        LUMEN_CHECK_EQ( Compiled.Error().Code, ERenderErrorCode::DependencyCycle );
    }

    LUMEN_TEST( FRenderGraphCompiler, BreaksTopologicalTiesByPassOrdinal )
    {
        FRenderGraphBuilder Builder( MakeCapacities( 4U, 0U ) );
        const auto PassZero  = Builder.AddPass( { .bHasSideEffects = true } );
        const auto PassOne   = Builder.AddPass( { .bHasSideEffects = true } );
        const auto PassTwo   = Builder.AddPass( { .bHasSideEffects = true } );
        const auto PassThree = Builder.AddPass( { .bHasSideEffects = true } );
        LUMEN_CHECK_HAS_VALUE( PassZero );
        LUMEN_CHECK_HAS_VALUE( PassOne );
        LUMEN_CHECK_HAS_VALUE( PassTwo );
        LUMEN_CHECK_HAS_VALUE( PassThree );
        if ( not PassZero or not PassOne or not PassTwo or not PassThree )
        {
            return;
        }

        LUMEN_CHECK_HAS_VALUE( Builder.AddDependency( *PassZero, *PassTwo ) );
        LUMEN_CHECK_HAS_VALUE( Builder.AddDependency( *PassOne, *PassThree ) );

        auto Compiled = std::move( Builder ).Compile();
        LUMEN_CHECK_HAS_VALUE( Compiled );
        if ( Compiled )
        {
            LUMEN_CHECK_EQ( Compiled->GetExecutionOrder().size(), 4U );
            LUMEN_CHECK_EQ( Compiled->GetExecutionOrder()[0], 0U );
            LUMEN_CHECK_EQ( Compiled->GetExecutionOrder()[1], 1U );
            LUMEN_CHECK_EQ( Compiled->GetExecutionOrder()[2], 2U );
            LUMEN_CHECK_EQ( Compiled->GetExecutionOrder()[3], 3U );
        }

        const UInt64 Expected = Fingerprint();
        for ( UInt32 Run = 0U; Run < 32U; ++Run )
        {
            LUMEN_CHECK_EQ( Fingerprint(), Expected );
        }
    }

    LUMEN_TEST( FRenderGraphCompiler, RestoresImportedFinalStateAndQueue )
    {
        RHI::Tests::IRHIConformanceDeviceFactory &Factory = RHI::Tests::GetRHIConformanceDeviceFactory();
        Factory.SetDefaultValidation( false );
        const auto Device = Factory.CreateDevice();
        LUMEN_CHECK_HAS_VALUE( Device );
        if ( not Device )
        {
            return;
        }
        const auto Buffer = ( *Device )->CreateBuffer( { .Size = 64ULL, .Usage = RHI::ERHIResourceUsage::UnorderedAccess } );
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
        LUMEN_CHECK_HAS_VALUE( Imported );
        if ( not Imported )
        {
            return;
        }
        const auto Pass = Builder.AddPass( {} );
        LUMEN_CHECK_HAS_VALUE( Pass );
        if ( not Pass )
        {
            return;
        }
        LUMEN_CHECK_HAS_VALUE( Builder.Write( *Pass, *Imported, RHI::ERHIResourceState::UnorderedAccess ) );
        LUMEN_CHECK_HAS_VALUE( Builder.Export( *Imported ) );

        auto Compiled = std::move( Builder ).Compile();
        LUMEN_CHECK_HAS_VALUE( Compiled );
        if ( not Compiled )
        {
            return;
        }

        /** INFO: Common -> UnorderedAccess at the pass, then UnorderedAccess -> ShaderResource for the owner. */
        LUMEN_CHECK_EQ( Compiled->GetTransitions().size(), 2U );
        LUMEN_CHECK_EQ( Compiled->GetTransitions()[0].Before, RHI::ERHIResourceState::Common );
        LUMEN_CHECK_EQ( Compiled->GetTransitions()[0].After, RHI::ERHIResourceState::UnorderedAccess );
        LUMEN_CHECK_EQ( Compiled->GetTransitions()[1].Before, RHI::ERHIResourceState::UnorderedAccess );
        LUMEN_CHECK_EQ( Compiled->GetTransitions()[1].After, RHI::ERHIResourceState::ShaderResource );
        LUMEN_CHECK_EQ( Compiled->GetTransitions()[1].DestinationPassOrdinal, InvalidRenderPassOrdinal );
        LUMEN_CHECK_TRUE( Compiled->GetTransitions()[0].RHITransition.HasValue() );

        FRenderGraphContext Context( std::move( *Compiled ) );
        const auto Transition = Context.ResolveTransition( 0U );
        LUMEN_CHECK_HAS_VALUE( Transition );
    }

    LUMEN_TEST( FRenderGraphCompiler, RecordsInitialQueueTransferInDestinationPass )
    {
        const auto Device = RHI::Tests::GetRHIConformanceDeviceFactory().CreateDevice( false );
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
                                                           .InitialQueue = RHI::ERHIQueueType::Compute,
                                                           .FinalQueue   = RHI::ERHIQueueType::Graphics,
                                                       } );
        const auto Pass     = Builder.AddPass( { .Queue = RHI::ERHIQueueType::Graphics, .bHasSideEffects = true } );
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
        LUMEN_CHECK_TRUE( Compiled->GetTransitions()[0].IsQueueTransfer() );
        LUMEN_CHECK_EQ( Compiled->GetTransitions()[0].SourceQueue, RHI::ERHIQueueType::Compute );
        LUMEN_CHECK_EQ( Compiled->GetTransitions()[0].DestinationQueue, RHI::ERHIQueueType::Graphics );
        LUMEN_CHECK_EQ( Compiled->GetTransitions()[0].RecordPassOrdinal, Pass->Ordinal );
        LUMEN_CHECK_EQ( Compiled->GetTransitions()[0].RecordPoint, ERenderTransitionRecordPoint::BeforePass );

        FRenderGraphContext Context( std::move( *Compiled ) );
        LUMEN_CHECK_HAS_VALUE( Context.ResolveTransition( 0U ) );
    }

    LUMEN_TEST( FRenderGraphCompiler, RejectsInitialQueueTransferWithoutStateChange )
    {
        const auto Device = RHI::Tests::GetRHIConformanceDeviceFactory().CreateDevice( false );
        LUMEN_CHECK_HAS_VALUE( Device );
        if ( not Device )
        {
            return;
        }
        const auto Buffer = ( *Device )->CreateBuffer( { .Size = 64ULL, .Usage = RHI::ERHIResourceUsage::ShaderResource } );
        LUMEN_CHECK_HAS_VALUE( Buffer );
        if ( not Buffer )
        {
            return;
        }

        FRenderGraphBuilder Builder( MakeCapacities( 1U, 1U ) );
        const auto Imported = Builder.Import( *Buffer, {
                                                           .InitialState = RHI::ERHIResourceState::ShaderResource,
                                                           .FinalState   = RHI::ERHIResourceState::ShaderResource,
                                                           .InitialQueue = RHI::ERHIQueueType::Compute,
                                                           .FinalQueue   = RHI::ERHIQueueType::Graphics,
                                                       } );
        const auto Pass     = Builder.AddPass( { .Queue = RHI::ERHIQueueType::Graphics, .bHasSideEffects = true } );
        LUMEN_CHECK_HAS_VALUE( Imported );
        LUMEN_CHECK_HAS_VALUE( Pass );
        if ( not Imported or not Pass )
        {
            return;
        }
        LUMEN_CHECK_HAS_VALUE( Builder.Read( *Pass, *Imported, RHI::ERHIResourceState::ShaderResource ) );

        auto Compiled = std::move( Builder ).Compile();
        LUMEN_CHECK_HAS_ERROR( Compiled );
        if ( Compiled )
        {
            return;
        }
        LUMEN_CHECK_EQ( Compiled.Error().Code, ERenderErrorCode::InvalidInitialQueue );
    }

    LUMEN_TEST( FRenderGraphCompiler, RejectsInvalidImportOwnership )
    {
        const auto Device = RHI::Tests::GetRHIConformanceDeviceFactory().CreateDevice();
        LUMEN_CHECK_HAS_VALUE( Device );
        if ( not Device )
        {
            return;
        }
        const auto Buffer = ( *Device )->CreateBuffer( { .Size = 64ULL, .Usage = RHI::ERHIResourceUsage::CopySource } );
        LUMEN_CHECK_HAS_VALUE( Buffer );
        if ( not Buffer )
        {
            return;
        }

        FRenderGraphBuilder Builder( MakeCapacities( 1U, 1U ) );
        const auto InvalidQueue = Builder.Import( *Buffer, {
                                                               .InitialState = RHI::ERHIResourceState::Common,
                                                               .InitialQueue = RHI::ERHIQueueType::Count,
                                                           } );
        LUMEN_CHECK_HAS_ERROR( InvalidQueue );
        if ( InvalidQueue )
        {
            return;
        }
        LUMEN_CHECK_EQ( InvalidQueue.Error().Code, ERenderErrorCode::InvalidResourceDescriptor );
    }

    LUMEN_TEST( FRenderGraphCompiler, RejectsCapacityOverflowStructurally )
    {
        FRenderGraphBuilder Builder( {
            .PassCount              = 1U,
            .ResourceCount          = 1U,
            .ReadCountPerPass       = 0U,
            .WriteCountPerPass      = 1U,
            .DependencyCountPerPass = 0U,
            .TransitionCount        = 0U,
            .QueueDependencyCount   = 0U,
        } );
        const auto Resource = Builder.CreateTransient( MakeBuffer( 64ULL ) );
        const auto Pass     = Builder.AddPass( {} );
        LUMEN_CHECK_HAS_VALUE( Resource );
        LUMEN_CHECK_HAS_VALUE( Pass );
        if ( not Resource or not Pass )
        {
            return;
        }

        LUMEN_CHECK_HAS_ERROR( Builder.CreateTransient( MakeBuffer( 64ULL ) ) );
        LUMEN_CHECK_HAS_ERROR( Builder.AddPass( {} ) );
        LUMEN_CHECK_HAS_ERROR( Builder.Read( *Pass, *Resource, RHI::ERHIResourceState::ShaderResource ) );
        LUMEN_CHECK_HAS_VALUE( Builder.Write( *Pass, *Resource, RHI::ERHIResourceState::RenderTarget ) );
        LUMEN_CHECK_HAS_ERROR( Builder.Write( *Pass, *Resource, RHI::ERHIResourceState::UnorderedAccess ) );
        LUMEN_CHECK_HAS_VALUE( Builder.Export( *Resource ) );

        auto Compiled = std::move( Builder ).Compile();
        LUMEN_CHECK_HAS_ERROR( Compiled );
        if ( Compiled )
        {
            return;
        }
        LUMEN_CHECK_EQ( Compiled.Error().Code, ERenderErrorCode::CapacityExceeded );
    }

    LUMEN_TEST( FRenderGraphCompiler, PlansQueueTransferAndMonotonicSignals )
    {
        FRenderGraphBuilder Builder( MakeCapacities( 2U, 1U ) );
        const auto Resource = Builder.CreateTransient( MakeBuffer( 64ULL ) );
        const auto Graphics = Builder.AddPass( { .Queue = RHI::ERHIQueueType::Graphics } );
        const auto Compute  = Builder.AddPass( { .Queue = RHI::ERHIQueueType::Compute, .bHasSideEffects = true } );
        LUMEN_CHECK_HAS_VALUE( Resource );
        LUMEN_CHECK_HAS_VALUE( Graphics );
        LUMEN_CHECK_HAS_VALUE( Compute );
        if ( not Resource or not Graphics or not Compute )
        {
            return;
        }

        LUMEN_CHECK_HAS_VALUE( Builder.Write( *Graphics, *Resource, RHI::ERHIResourceState::RenderTarget ) );
        LUMEN_CHECK_HAS_VALUE( Builder.Read( *Compute, *Resource, RHI::ERHIResourceState::UnorderedAccess ) );
        LUMEN_CHECK_HAS_VALUE( Builder.Export( *Resource ) );

        auto Compiled = std::move( Builder ).Compile();
        LUMEN_CHECK_HAS_VALUE( Compiled );
        if ( not Compiled )
        {
            return;
        }

        LUMEN_CHECK_EQ( Compiled->GetQueueSegments().size(), 2U );
        LUMEN_CHECK_EQ( Compiled->GetQueueSegments()[0].Queue, RHI::ERHIQueueType::Graphics );
        LUMEN_CHECK_EQ( Compiled->GetQueueSegments()[1].Queue, RHI::ERHIQueueType::Compute );
        LUMEN_CHECK_EQ( Compiled->GetQueueSegments()[0].SignalValue, 1ULL );
        LUMEN_CHECK_EQ( Compiled->GetQueueSegments()[1].SignalValue, 1ULL );
        LUMEN_CHECK_EQ( Compiled->GetQueueDependencies().size(), 1U );
        LUMEN_CHECK_EQ( Compiled->GetQueueDependencies()[0].SourceSegmentIndex, 0U );
        LUMEN_CHECK_EQ( Compiled->GetQueueDependencies()[0].DestinationSegmentIndex, 1U );
        LUMEN_CHECK_EQ( Compiled->GetQueueDependencies()[0].WaitValue, 1ULL );

        LUMEN_CHECK_EQ( Compiled->GetTransitions().size(), 2U );
        LUMEN_CHECK_TRUE( Compiled->GetTransitions()[1].IsQueueTransfer() );
        LUMEN_CHECK_EQ( Compiled->GetTransitions()[1].SourceQueue, RHI::ERHIQueueType::Graphics );
        LUMEN_CHECK_EQ( Compiled->GetTransitions()[1].DestinationQueue, RHI::ERHIQueueType::Compute );
    }

    LUMEN_TEST( FRenderGraphCompiler, RejectsQueueTransferWithoutStateChange )
    {
        FRenderGraphBuilder Builder( MakeCapacities( 2U, 1U ) );
        const auto Resource = Builder.CreateTransient( MakeBuffer( 64ULL ) );
        const auto Graphics = Builder.AddPass( { .Queue = RHI::ERHIQueueType::Graphics } );
        const auto Compute  = Builder.AddPass( { .Queue = RHI::ERHIQueueType::Compute, .bHasSideEffects = true } );
        LUMEN_CHECK_HAS_VALUE( Resource );
        LUMEN_CHECK_HAS_VALUE( Graphics );
        LUMEN_CHECK_HAS_VALUE( Compute );
        if ( not Resource or not Graphics or not Compute )
        {
            return;
        }

        LUMEN_CHECK_HAS_VALUE( Builder.Write( *Graphics, *Resource, RHI::ERHIResourceState::ShaderResource ) );
        LUMEN_CHECK_HAS_VALUE( Builder.Read( *Compute, *Resource, RHI::ERHIResourceState::ShaderResource ) );
        LUMEN_CHECK_HAS_VALUE( Builder.Export( *Resource ) );

        auto Compiled = std::move( Builder ).Compile();
        LUMEN_CHECK_HAS_ERROR( Compiled );
        if ( Compiled )
        {
            return;
        }
        LUMEN_CHECK_EQ( Compiled.Error().Code, ERenderErrorCode::UnsupportedQueueTransfer );
    }

    LUMEN_TEST( FRenderGraphCompiler, RejectsInvalidHandlesAndStates )
    {
        FRenderGraphBuilder Builder( MakeCapacities( 2U, 1U ) );
        const auto Resource = Builder.CreateTransient( MakeBuffer( 64ULL ) );
        const auto Pass     = Builder.AddPass( {} );
        LUMEN_CHECK_HAS_VALUE( Resource );
        LUMEN_CHECK_HAS_VALUE( Pass );
        if ( not Resource or not Pass )
        {
            return;
        }

        const FRenderPassHandle InvalidPass{ .Ordinal = 7U };
        const FRenderResourceHandle StaleResource{ .Index = Resource->Index, .Generation = Resource->Generation + 1U };
        LUMEN_CHECK_HAS_ERROR( Builder.Read( InvalidPass, *Resource, RHI::ERHIResourceState::ShaderResource ) );
        LUMEN_CHECK_HAS_ERROR( Builder.Read( *Pass, StaleResource, RHI::ERHIResourceState::ShaderResource ) );
        LUMEN_CHECK_HAS_ERROR( Builder.Read( *Pass, *Resource, RHI::ERHIResourceState::Undefined ) );
        LUMEN_CHECK_HAS_VALUE( Builder.Write( *Pass, *Resource, RHI::ERHIResourceState::RenderTarget ) );
        LUMEN_CHECK_HAS_ERROR( Builder.Write( *Pass, *Resource, RHI::ERHIResourceState::UnorderedAccess ) );

        const FRenderResourceDesc InvalidDesc{ .Creation = RHI::FRHIBufferDesc{}, .Requirements = {} };
        LUMEN_CHECK_HAS_ERROR( Builder.CreateTransient( InvalidDesc ) );
        LUMEN_CHECK_HAS_ERROR( Builder.AddPass( { .Queue = RHI::ERHIQueueType::Count } ) );
    }

} // namespace

} // namespace Lumen
