/**
 * @file RenderGraphContext.cpp
 * @brief Implements transient binding and deferred RHI barrier materialization.
 */

#include <RenderGraph/RenderGraphContext.hpp>
#include <RenderGraph/RenderGraphTransition.hpp>

#include <utility>

namespace
{

/** @brief A materialized buffer must cover the declared creation parameters before it may be bound. */
[[nodiscard]] Lumen::Bool IsCompatible ( const Lumen::RHI::FRHIBufferDesc &InDeclared,
                                         const Lumen::RHI::FRHIBufferDesc &InActual ) noexcept
{
    return InActual.Size >= InDeclared.Size and ( InActual.Usage & InDeclared.Usage ) == InDeclared.Usage and
           InActual.MemoryClass == InDeclared.MemoryClass and InActual.InitialState == InDeclared.InitialState;
}

/** @brief A materialized texture must match the declared immutable parameters before it may be bound. */
[[nodiscard]] Lumen::Bool IsCompatible ( const Lumen::RHI::FRHITextureDesc &InDeclared,
                                         const Lumen::RHI::FRHITextureDesc &InActual ) noexcept
{
    return InActual.Extent.X == InDeclared.Extent.X and InActual.Extent.Y == InDeclared.Extent.Y and
           InActual.Extent.Z == InDeclared.Extent.Z and InActual.Format == InDeclared.Format and
           InActual.MipCount == InDeclared.MipCount and InActual.LayerCount == InDeclared.LayerCount and
           InActual.SampleCount == InDeclared.SampleCount and ( InActual.Usage & InDeclared.Usage ) == InDeclared.Usage and
           InActual.MemoryClass == InDeclared.MemoryClass and InActual.InitialState == InDeclared.InitialState;
}

[[nodiscard]] Lumen::FRenderError MakeDescriptorError ( const Lumen::FRenderResourceHandle InResource ) noexcept
{
    return Lumen::FRenderError{ .Code = Lumen::ERenderErrorCode::InvalidResourceDescriptor, .ResourceIndex = InResource.Index };
}

[[nodiscard]] Lumen::FRenderError MakeBindingError ( const Lumen::ERenderErrorCode InCode,
                                                     const Lumen::FRenderResourceHandle InResource ) noexcept
{
    return Lumen::FRenderError{ .Code = InCode, .ResourceIndex = InResource.Index };
}

} // namespace

Lumen::FRenderGraphContext::FRenderGraphContext ( FCompiledRenderGraph &&InGraph ) noexcept : Graph( std::move( InGraph ) )
{
    Bindings.reserve( Graph.GetResources().size() );
    for ( const FRenderResource &Resource : Graph.GetResources() )
    {
        Bindings.push_back( Resource.Binding );
    }
}

Lumen::FRenderExpected Lumen::FRenderGraphContext::BindTransient ( const FRenderResourceHandle InResource,
                                                                   TSharedRef<RHI::IRHIBuffer> InBuffer ) noexcept
{
    LUMEN_EXPECT( Resource, Graph.GetResource( InResource ) );
    if ( Resource->bImported )
    {
        return TUnexpected<FRenderError>( MakeBindingError( ERenderErrorCode::InvalidAccess, InResource ) );
    }
    if ( not Resource->bActive )
    {
        return TUnexpected<FRenderError>( MakeBindingError( ERenderErrorCode::ResourceCulled, InResource ) );
    }
    if ( not std::holds_alternative<RHI::FRHIBufferDesc>( Resource->Desc.Creation ) )
    {
        return TUnexpected<FRenderError>( MakeBindingError( ERenderErrorCode::ResourceTypeMismatch, InResource ) );
    }
    if ( not IsCompatible( std::get<RHI::FRHIBufferDesc>( Resource->Desc.Creation ), InBuffer->GetDesc() ) )
    {
        return TUnexpected<FRenderError>( MakeDescriptorError( InResource ) );
    }
    Bindings[InResource.Index].Resource = TSharedPtr<RHI::IRHIBuffer>( InBuffer );
    return {};
}

Lumen::FRenderExpected Lumen::FRenderGraphContext::BindTransient ( const FRenderResourceHandle InResource,
                                                                   TSharedRef<RHI::IRHITexture> InTexture ) noexcept
{
    LUMEN_EXPECT( Resource, Graph.GetResource( InResource ) );
    if ( Resource->bImported )
    {
        return TUnexpected<FRenderError>( MakeBindingError( ERenderErrorCode::InvalidAccess, InResource ) );
    }
    if ( not Resource->bActive )
    {
        return TUnexpected<FRenderError>( MakeBindingError( ERenderErrorCode::ResourceCulled, InResource ) );
    }
    if ( not std::holds_alternative<RHI::FRHITextureDesc>( Resource->Desc.Creation ) )
    {
        return TUnexpected<FRenderError>( MakeBindingError( ERenderErrorCode::ResourceTypeMismatch, InResource ) );
    }
    if ( not IsCompatible( std::get<RHI::FRHITextureDesc>( Resource->Desc.Creation ), InTexture->GetDesc() ) )
    {
        return TUnexpected<FRenderError>( MakeDescriptorError( InResource ) );
    }
    Bindings[InResource.Index].Resource = TSharedPtr<RHI::IRHITexture>( InTexture );
    return {};
}

Lumen::TExpected<Lumen::FRenderResourceBinding, Lumen::FRenderError>
Lumen::FRenderGraphContext::GetResourceBinding ( const FRenderResourceHandle InResource ) const noexcept
{
    LUMEN_EXPECT( Resource, Graph.GetResource( InResource ) );
    return Bindings[InResource.Index];
}

Lumen::TExpected<Lumen::RHI::FRHITransition, Lumen::FRenderError>
Lumen::FRenderGraphContext::ResolveTransition ( const USize InTransitionIndex ) const noexcept
{
    if ( InTransitionIndex >= Graph.GetTransitions().size() )
    {
        return TUnexpected<FRenderError>( FRenderError{ .Code = ERenderErrorCode::TransitionOutOfRange } );
    }

    const FRenderTransition &Transition = Graph.GetTransitions()[InTransitionIndex];
    LUMEN_EXPECT( Resource, Graph.GetResource( Transition.Resource ) );
    const FRenderResourceBinding &Binding = Bindings[Transition.Resource.Index];

    TExpected<RHI::FRHITransition, FRenderError> RHITransition =
        MakeBoundTransition( Transition.Resource, Resource->Desc, Binding, Transition.Range, Transition.Before, Transition.After,
                             Transition.SourceQueue, Transition.DestinationQueue, ERenderErrorCode::InvalidTransition );
    if ( not RHITransition ) [[unlikely]]
    {
        return MakeUnexpected( std::move( RHITransition ).Error() );
    }
    return std::move( *RHITransition );
}
