/**
 * @file RenderGraphBuilder.cpp
 * @brief Implements checked, capacity-bounded render graph declaration.
 */

#include <RenderGraph/RenderGraphBuilder.hpp>

#include <RenderGraph/RenderGraphCompiler.hpp>

#include <utility>

namespace
{

Lumen::TUnexpected<Lumen::FRenderError>
MakeRenderError ( const Lumen::ERenderErrorCode InCode,
                  const Lumen::UInt32 InPass        = Lumen::InvalidRenderPassOrdinal,
                  const Lumen::UInt32 InResource    = Lumen::InvalidRenderResourceIndex,
                  const Lumen::UInt32 InRelatedPass = Lumen::InvalidRenderPassOrdinal ) noexcept
{
    return Lumen::TUnexpected<Lumen::FRenderError>( Lumen::FRenderError{
        .Code = InCode, .PassOrdinal = InPass, .ResourceIndex = InResource, .RelatedPassOrdinal = InRelatedPass } );
}

Lumen::Bool IsQueueValid ( const Lumen::RHI::ERHIQueueType InQueue ) noexcept
{
    return InQueue < Lumen::RHI::ERHIQueueType::Count;
}

Lumen::Bool IsDeclaredStateValid ( const Lumen::RHI::ERHIResourceState InState ) noexcept
{
    return InState != Lumen::RHI::ERHIResourceState::Undefined and Lumen::RHI::IsValidRHIResourceState( InState );
}

} // namespace

Lumen::FRenderGraphBuilder::FRenderGraphBuilder ( const FRenderGraphCapacities &InCapacities ) noexcept : Graph( InCapacities )
{
}

Lumen::TExpected<Lumen::FRenderResourceHandle, Lumen::FRenderError>
Lumen::FRenderGraphBuilder::AddResource ( FRenderResource &&InResource ) noexcept
{
    if ( bCompiled )
    {
        return MakeRenderError( ERenderErrorCode::GraphAlreadyCompiled );
    }
    if ( Resources.size() >= Graph.Capacities.ResourceCount or Graph.Resources.size() >= InvalidRenderResourceIndex or
         Graph.NextResourceGeneration == 0U )
    {
        return MakeRenderError( ERenderErrorCode::CapacityExceeded );
    }

    InResource.Handle = FRenderResourceHandle{ static_cast<UInt32>( Graph.Resources.size() ), Graph.NextResourceGeneration };
    ++Graph.NextResourceGeneration;
    const FRenderResourceHandle Handle = InResource.Handle;
    Graph.Resources.push_back( std::move( InResource ) );
    return Handle;
}

Lumen::TExpected<Lumen::FRenderResourceHandle, Lumen::FRenderError>
Lumen::FRenderGraphBuilder::CreateTransient ( const FRenderResourceDesc &InDesc ) noexcept
{
    Bool bValidDescriptor = InDesc.Requirements.ToAllocationDesc().HasValue();
    RHI::FRHISubresourceRange Range;

    if ( std::holds_alternative<RHI::FRHIBufferDesc>( InDesc.Creation ) )
    {
        bValidDescriptor = bValidDescriptor and std::get<RHI::FRHIBufferDesc>( InDesc.Creation ).Validate().HasValue();
    }
    else
    {
        const RHI::FRHITextureDesc &Texture = std::get<RHI::FRHITextureDesc>( InDesc.Creation );
        bValidDescriptor                    = bValidDescriptor and Texture.Validate().HasValue();
        Range.MipCount                      = Texture.MipCount;
        Range.LayerCount                    = Texture.LayerCount;
    }

    if ( not bValidDescriptor )
    {
        return MakeRenderError( ERenderErrorCode::InvalidResourceDescriptor );
    }

    FRenderResource Resource;
    Resource.Desc  = InDesc;
    Resource.Range = Range;
    return AddResource( std::move( Resource ) );
}

Lumen::TExpected<Lumen::FRenderResourceHandle, Lumen::FRenderError>
Lumen::FRenderGraphBuilder::Import ( TSharedRef<RHI::IRHIBuffer> InBuffer, const FRenderImportedResourceDesc &InDesc ) noexcept
{
    if ( not IsDeclaredStateValid( InDesc.InitialState ) or not IsDeclaredStateValid( InDesc.FinalState ) or
         not IsQueueValid( InDesc.InitialQueue ) or not IsQueueValid( InDesc.FinalQueue ) )
    {
        return MakeRenderError( ERenderErrorCode::InvalidResourceDescriptor );
    }

    FRenderResource Resource;
    Resource.Desc.Creation        = InBuffer->GetDesc();
    Resource.Desc.Requirements    = InBuffer->GetRequirements();
    Resource.Binding.Resource     = TSharedPtr<RHI::IRHIBuffer>( InBuffer );
    Resource.Binding.InitialState = InDesc.InitialState;
    Resource.Binding.FinalState   = InDesc.FinalState;
    Resource.Binding.InitialQueue = InDesc.InitialQueue;
    Resource.Binding.FinalQueue   = InDesc.FinalQueue;
    Resource.bImported            = true;
    return AddResource( std::move( Resource ) );
}

Lumen::TExpected<Lumen::FRenderResourceHandle, Lumen::FRenderError>
Lumen::FRenderGraphBuilder::Import ( TSharedRef<RHI::IRHITexture> InTexture, const FRenderImportedResourceDesc &InDesc ) noexcept
{
    if ( not IsDeclaredStateValid( InDesc.InitialState ) or not IsDeclaredStateValid( InDesc.FinalState ) or
         not IsQueueValid( InDesc.InitialQueue ) or not IsQueueValid( InDesc.FinalQueue ) )
    {
        return MakeRenderError( ERenderErrorCode::InvalidResourceDescriptor );
    }

    const RHI::FRHITextureDesc &Texture = InTexture->GetDesc();
    FRenderResource Resource;
    Resource.Desc.Creation        = Texture;
    Resource.Desc.Requirements    = InTexture->GetRequirements();
    Resource.Binding.Resource     = TSharedPtr<RHI::IRHITexture>( InTexture );
    Resource.Binding.InitialState = InDesc.InitialState;
    Resource.Binding.FinalState   = InDesc.FinalState;
    Resource.Binding.InitialQueue = InDesc.InitialQueue;
    Resource.Binding.FinalQueue   = InDesc.FinalQueue;
    Resource.Range.MipCount       = Texture.MipCount;
    Resource.Range.LayerCount     = Texture.LayerCount;
    Resource.bImported            = true;
    return AddResource( std::move( Resource ) );
}

Lumen::TExpected<Lumen::FRenderPassHandle, Lumen::FRenderError>
Lumen::FRenderGraphBuilder::AddPass ( const FRenderPassDesc &InDesc ) noexcept
{
    if ( bCompiled )
    {
        return MakeRenderError( ERenderErrorCode::GraphAlreadyCompiled );
    }
    if ( not IsQueueValid( InDesc.Queue ) )
    {
        return MakeRenderError( ERenderErrorCode::InvalidQueue );
    }
    if ( Graph.Passes.size() >= Graph.Capacities.PassCount or Graph.Passes.size() >= InvalidRenderPassOrdinal )
    {
        return MakeRenderError( ERenderErrorCode::CapacityExceeded );
    }

    FRenderPass Pass;
    Pass.Desc    = InDesc;
    Pass.Ordinal = static_cast<UInt32>( Graph.Passes.size() );
    Pass.Reads.reserve( Graph.Capacities.ReadCountPerPass );
    Pass.Writes.reserve( Graph.Capacities.WriteCountPerPass );
    Pass.Dependencies.reserve( Graph.Capacities.DependencyCountPerPass );
    Pass.Dependents.reserve( Graph.Capacities.DependencyCountPerPass );
    Graph.Passes.push_back( std::move( Pass ) );
    Graph.PassStates.push_back( FRenderGraph::FPassCompilerState{} );
    return FRenderPassHandle{ Graph.Passes.back().Ordinal };
}

Lumen::FRenderExpected Lumen::FRenderGraphBuilder::ValidatePass ( const FRenderPassHandle InPass ) const noexcept
{
    if ( bCompiled )
    {
        return MakeRenderError( ERenderErrorCode::GraphAlreadyCompiled );
    }
    if ( not InPass.IsValid() or InPass.Ordinal >= Graph.Passes.size() )
    {
        return MakeRenderError( ERenderErrorCode::InvalidPassHandle, InPass.Ordinal );
    }
    return {};
}

Lumen::FRenderExpected Lumen::FRenderGraphBuilder::ValidateResource ( const FRenderResourceHandle InResource ) const noexcept
{
    if ( bCompiled )
    {
        return MakeRenderError( ERenderErrorCode::GraphAlreadyCompiled );
    }
    if ( not InResource.IsValid() or InResource.Index >= Graph.Resources.size() or
         Graph.Resources[InResource.Index].Handle != InResource )
    {
        return MakeRenderError( ERenderErrorCode::InvalidResourceHandle, InvalidRenderPassOrdinal, InResource.Index );
    }
    return {};
}

Lumen::FRenderExpected Lumen::FRenderGraphBuilder::AddAccess ( const FRenderPassHandle InPass,
                                                               const FRenderResourceHandle InResource,
                                                               const RHI::ERHIResourceState InState,
                                                               const Bool bWrite ) noexcept
{
    LUMEN_TRY_EXPECT_ERROR( ValidatePass( InPass ) );
    LUMEN_TRY_EXPECT_ERROR( ValidateResource( InResource ) );
    if ( not IsDeclaredStateValid( InState ) )
    {
        return MakeRenderError( ERenderErrorCode::InvalidState, InPass.Ordinal, InResource.Index );
    }

    FRenderPass &Pass = Graph.Passes[InPass.Ordinal];
    for ( const FRenderResourceAccess &Access : Pass.Reads )
    {
        if ( Access.Resource == InResource )
        {
            return MakeRenderError( ERenderErrorCode::InvalidAccess, InPass.Ordinal, InResource.Index );
        }
    }
    for ( const FRenderResourceAccess &Access : Pass.Writes )
    {
        if ( Access.Resource == InResource )
        {
            return MakeRenderError( bWrite ? ERenderErrorCode::AmbiguousWrite : ERenderErrorCode::InvalidAccess, InPass.Ordinal,
                                    InResource.Index );
        }
    }

    TVector<FRenderResourceAccess> &Accesses = bWrite ? Pass.Writes : Pass.Reads;
    const USize Capacity                     = bWrite ? Graph.Capacities.WriteCountPerPass : Graph.Capacities.ReadCountPerPass;
    if ( Accesses.size() >= Capacity )
    {
        return MakeRenderError( ERenderErrorCode::CapacityExceeded, InPass.Ordinal, InResource.Index );
    }
    Accesses.push_back( FRenderResourceAccess{ InResource, InState } );
    return {};
}

Lumen::FRenderExpected Lumen::FRenderGraphBuilder::Read ( const FRenderPassHandle InPass,
                                                          const FRenderResourceHandle InResource,
                                                          const RHI::ERHIResourceState InState ) noexcept
{
    return AddAccess( InPass, InResource, InState, false );
}

Lumen::FRenderExpected Lumen::FRenderGraphBuilder::Write ( const FRenderPassHandle InPass,
                                                           const FRenderResourceHandle InResource,
                                                           const RHI::ERHIResourceState InState ) noexcept
{
    return AddAccess( InPass, InResource, InState, true );
}

Lumen::FRenderExpected Lumen::FRenderGraphBuilder::AddDependency ( const FRenderPassHandle InBefore,
                                                                   const FRenderPassHandle InAfter ) noexcept
{
    LUMEN_TRY_EXPECT_ERROR( ValidatePass( InBefore ) );
    LUMEN_TRY_EXPECT_ERROR( ValidatePass( InAfter ) );
    if ( InBefore.Ordinal == InAfter.Ordinal )
    {
        return MakeRenderError( ERenderErrorCode::DependencyCycle, InBefore.Ordinal, InvalidRenderResourceIndex,
                                InAfter.Ordinal );
    }

    FRenderPass &Before = Graph.Passes[InBefore.Ordinal];
    FRenderPass &After  = Graph.Passes[InAfter.Ordinal];
    for ( const UInt32 Dependency : Before.Dependents )
    {
        if ( Dependency == InAfter.Ordinal )
        {
            return {};
        }
    }
    if ( Before.Dependents.size() >= Graph.Capacities.DependencyCountPerPass or
         After.Dependencies.size() >= Graph.Capacities.DependencyCountPerPass )
    {
        return MakeRenderError( ERenderErrorCode::CapacityExceeded, InBefore.Ordinal, InvalidRenderResourceIndex,
                                InAfter.Ordinal );
    }
    Before.Dependents.push_back( InAfter.Ordinal );
    After.Dependencies.push_back( InBefore.Ordinal );
    return {};
}

Lumen::FRenderExpected Lumen::FRenderGraphBuilder::Export ( const FRenderResourceHandle InResource ) noexcept
{
    LUMEN_TRY_EXPECT_ERROR( ValidateResource( InResource ) );
    Graph.Resources[InResource.Index].bExported = true;
    return {};
}

Lumen::TExpected<Lumen::FCompiledRenderGraph, Lumen::FRenderError> Lumen::FRenderGraphBuilder::Compile () && noexcept
{
    if ( bCompiled )
    {
        return MakeRenderError( ERenderErrorCode::GraphAlreadyCompiled );
    }
    bCompiled = true;
    return FRenderGraphCompiler::Compile( std::move( Graph ) );
}
