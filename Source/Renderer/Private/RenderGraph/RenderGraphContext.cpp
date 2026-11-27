/**
 * @file RenderGraphContext.cpp
 * @brief Implements transient binding and deferred RHI barrier materialization.
 */

#include <RenderGraph/RenderGraphContext.hpp>

#include <utility>

Lumen::FRenderGraphContext::FRenderGraphContext ( const FCompiledRenderGraph &InGraph ) noexcept : Graph( &InGraph )
{
    Bindings.reserve( InGraph.GetResources().size() );
    for ( const FRenderResource &Resource : InGraph.GetResources() )
    {
        Bindings.push_back( Resource.Binding );
    }
}

Lumen::FRenderExpected Lumen::FRenderGraphContext::BindTransient ( const FRenderResourceHandle InResource,
                                                                   TSharedRef<RHI::IRHIBuffer> InBuffer ) noexcept
{
    LUMEN_EXPECT( Resource, Graph->GetResource( InResource ) );
    if ( Resource->bImported )
    {
        return TUnexpected<FRenderError>(
            FRenderError{ .Code = ERenderErrorCode::InvalidAccess, .ResourceIndex = InResource.Index } );
    }
    if ( not Resource->bActive )
    {
        return TUnexpected<FRenderError>(
            FRenderError{ .Code = ERenderErrorCode::ResourceCulled, .ResourceIndex = InResource.Index } );
    }
    if ( not std::holds_alternative<RHI::FRHIBufferDesc>( Resource->Desc.Creation ) )
    {
        return TUnexpected<FRenderError>(
            FRenderError{ .Code = ERenderErrorCode::ResourceTypeMismatch, .ResourceIndex = InResource.Index } );
    }
    Bindings[InResource.Index].Resource = TSharedPtr<RHI::IRHIBuffer>( InBuffer );
    return {};
}

Lumen::FRenderExpected Lumen::FRenderGraphContext::BindTransient ( const FRenderResourceHandle InResource,
                                                                   TSharedRef<RHI::IRHITexture> InTexture ) noexcept
{
    LUMEN_EXPECT( Resource, Graph->GetResource( InResource ) );
    if ( Resource->bImported )
    {
        return TUnexpected<FRenderError>(
            FRenderError{ .Code = ERenderErrorCode::InvalidAccess, .ResourceIndex = InResource.Index } );
    }
    if ( not Resource->bActive )
    {
        return TUnexpected<FRenderError>(
            FRenderError{ .Code = ERenderErrorCode::ResourceCulled, .ResourceIndex = InResource.Index } );
    }
    if ( not std::holds_alternative<RHI::FRHITextureDesc>( Resource->Desc.Creation ) )
    {
        return TUnexpected<FRenderError>(
            FRenderError{ .Code = ERenderErrorCode::ResourceTypeMismatch, .ResourceIndex = InResource.Index } );
    }
    Bindings[InResource.Index].Resource = TSharedPtr<RHI::IRHITexture>( InTexture );
    return {};
}

Lumen::TExpected<Lumen::FRenderResourceBinding, Lumen::FRenderError>
Lumen::FRenderGraphContext::GetResourceBinding ( const FRenderResourceHandle InResource ) const noexcept
{
    LUMEN_EXPECT( Resource, Graph->GetResource( InResource ) );
    return Bindings[InResource.Index];
}

Lumen::TExpected<Lumen::RHI::FRHITransition, Lumen::FRenderError>
Lumen::FRenderGraphContext::ResolveTransition ( const USize InTransitionIndex ) const noexcept
{
    if ( InTransitionIndex >= Graph->GetTransitions().size() )
    {
        return TUnexpected<FRenderError>( FRenderError{ .Code = ERenderErrorCode::TransitionOutOfRange } );
    }

    const FRenderTransition &Transition = Graph->GetTransitions()[InTransitionIndex];
    LUMEN_EXPECT( Resource, Graph->GetResource( Transition.Resource ) );
    const FRenderResourceBinding &Binding = Bindings[Transition.Resource.Index];

    RHI::FRHIExpected Validation;
    if ( std::holds_alternative<RHI::FRHIBufferDesc>( Resource->Desc.Creation ) )
    {
        if ( not std::holds_alternative<TSharedPtr<RHI::IRHIBuffer>>( Binding.Resource ) )
        {
            return TUnexpected<FRenderError>(
                FRenderError{ .Code = ERenderErrorCode::ResourceTypeMismatch, .ResourceIndex = Transition.Resource.Index } );
        }
        RHI::FRHIBufferTransition BufferTransition;
        BufferTransition.Buffer           = std::get<TSharedPtr<RHI::IRHIBuffer>>( Binding.Resource );
        BufferTransition.Before           = Transition.Before;
        BufferTransition.After            = Transition.After;
        BufferTransition.SourceQueue      = Transition.SourceQueue;
        BufferTransition.DestinationQueue = Transition.DestinationQueue;
        if ( not BufferTransition.Buffer )
        {
            return TUnexpected<FRenderError>(
                FRenderError{ .Code = ERenderErrorCode::TransientResourceNotBound, .ResourceIndex = Transition.Resource.Index } );
        }
        Validation = BufferTransition.Validate();
        if ( not Validation )
        {
            return TUnexpected<FRenderError>(
                FRenderError{ .Code = ERenderErrorCode::InvalidTransition, .ResourceIndex = Transition.Resource.Index } );
        }
        return RHI::FRHITransition( BufferTransition );
    }

    if ( not std::holds_alternative<TSharedPtr<RHI::IRHITexture>>( Binding.Resource ) )
    {
        return TUnexpected<FRenderError>(
            FRenderError{ .Code = ERenderErrorCode::ResourceTypeMismatch, .ResourceIndex = Transition.Resource.Index } );
    }
    RHI::FRHITextureTransition TextureTransition;
    TextureTransition.Texture          = std::get<TSharedPtr<RHI::IRHITexture>>( Binding.Resource );
    TextureTransition.Range            = Transition.Range;
    TextureTransition.Before           = Transition.Before;
    TextureTransition.After            = Transition.After;
    TextureTransition.SourceQueue      = Transition.SourceQueue;
    TextureTransition.DestinationQueue = Transition.DestinationQueue;
    if ( not TextureTransition.Texture )
    {
        return TUnexpected<FRenderError>(
            FRenderError{ .Code = ERenderErrorCode::TransientResourceNotBound, .ResourceIndex = Transition.Resource.Index } );
    }
    Validation = TextureTransition.Validate();
    if ( not Validation )
    {
        return TUnexpected<FRenderError>(
            FRenderError{ .Code = ERenderErrorCode::InvalidTransition, .ResourceIndex = Transition.Resource.Index } );
    }
    return RHI::FRHITransition( TextureTransition );
}
