/**
 * @file RHIValidationCommandList.cpp
 * @brief Implements command recording validation and transparent forwarding.
 */
#include "RHIValidationCommandList.hpp"
#include "RHIValidationCommandAllocator.hpp"
#include <Queries/IRHIQueryPool.hpp>
#include <Validation/RHIValidation.hpp>
#include <variant>

Lumen::FRHIValidationCommandList::FRHIValidationCommandList ( TSharedRef<IRHICommandList> InUnderlying,
                                                              TSharedPtr<FRHIValidationCommandAllocator> InAllocator,
                                                              TSharedPtr<FRHIValidationState> InState ) noexcept
    : Underlying( std::move( InUnderlying ) ), Allocator( std::move( InAllocator ) ), State( std::move( InState ) )
{
    State->Track( *Underlying, ERHIValidationObjectKind::CommandList );
    Allocator->RegisterList( *this );
}

Lumen::FRHIValidationCommandList::~FRHIValidationCommandList () noexcept
{
    Allocator->UnregisterList( *this );
    State->Untrack( *Underlying );
}

LUMEN_RHI_VALIDATION_OBJECT_FORWARDERS( FRHIValidationCommandList )

Lumen::ERHIQueueType Lumen::FRHIValidationCommandList::GetQueueType () const noexcept
{
    return Underlying->GetQueueType();
}

Lumen::ERHICommandListState Lumen::FRHIValidationCommandList::GetState () const noexcept
{
    return Underlying->GetState();
}

Lumen::FRHITimelineValue Lumen::FRHIValidationCommandList::GetCompletion () const noexcept
{
    return Underlying->GetCompletion();
}

const Lumen::FRHICommandStream &Lumen::FRHIValidationCommandList::GetStream () const noexcept
{
    return Underlying->GetStream();
}

Lumen::TExpected<void, Lumen::FRHIError>
Lumen::FRHIValidationCommandList::CheckRecording ( const Bool bAllowRendering ) const noexcept
{
    const ERHICommandListState Current = GetState();
    if ( Current == ERHICommandListState::Recording or ( bAllowRendering and Current == ERHICommandListState::Rendering ) )
    {
        return {};
    }
    return State->GetReporter()->Reject( ERHIValidationMessageCode::InvalidCommandState, ERHIErrorCondition::Command,
                                         "Command is outside a compatible recording scope", this );
}

Lumen::TExpected<void, Lumen::FRHIError> Lumen::FRHIValidationCommandList::CheckRendering () const noexcept
{
    if ( GetState() == ERHICommandListState::Rendering )
    {
        return {};
    }
    return State->GetReporter()->Reject( ERHIValidationMessageCode::InvalidCommandState, ERHIErrorCondition::Command,
                                         "Command requires an active rendering scope", this );
}

Lumen::TExpected<void, Lumen::FRHIError>
Lumen::FRHIValidationCommandList::ValidateBuffer ( const IRHIBuffer &InBuffer ) const noexcept
{
    return State->ValidateObject( InBuffer, ERHIValidationObjectKind::Buffer );
}

Lumen::TExpected<void, Lumen::FRHIError>
Lumen::FRHIValidationCommandList::ValidateTexture ( const IRHITexture &InTexture ) const noexcept
{
    return State->ValidateObject( InTexture, ERHIValidationObjectKind::Texture );
}

Lumen::TExpected<void, Lumen::FRHIError> Lumen::FRHIValidationCommandList::CheckDraw ( const ERHIPipelineType InType,
                                                                                       const Bool bRendering ) const noexcept
{
    return ( bRendering ? CheckRendering() : CheckRecording( false ) )
        .AndThen( [this, InType] () noexcept -> TExpected<void, FRHIError>
    {
        if ( not BoundPipeline or *BoundPipeline != InType )
        {
            return State->GetReporter()->Reject( ERHIValidationMessageCode::InvalidCommandState, ERHIErrorCondition::Pipeline,
                                                 "Command has no compatible bound pipeline", this );
        }
        return {};
    } );
}

Lumen::TExpected<void, Lumen::FRHIError> Lumen::FRHIValidationCommandList::Begin () noexcept
{
    if ( GetState() != ERHICommandListState::Initial )
    {
        return State->GetReporter()->Reject( ERHIValidationMessageCode::InvalidCommandState, ERHIErrorCondition::Command,
                                             "Only an initial command list can begin", this );
    }
    return Underlying->Begin();
}

Lumen::TExpected<void, Lumen::FRHIError> Lumen::FRHIValidationCommandList::End () noexcept
{
    if ( GetState() != ERHICommandListState::Recording or ActiveQueryPool != nullptr or DebugDepth != 0U )
    {
        return State->GetReporter()->Reject( ERHIValidationMessageCode::InvalidCommandState, ERHIErrorCondition::Command,
                                             "Command list has an open rendering, query, or debug scope", this );
    }
    return Underlying->End();
}

Lumen::TExpected<void, Lumen::FRHIError> Lumen::FRHIValidationCommandList::Reset () noexcept
{
    if ( GetState() == ERHICommandListState::Pending or ActiveQueryPool != nullptr )
    {
        return State->GetReporter()->Reject( ERHIValidationMessageCode::InvalidCommandState, ERHIErrorCondition::Command,
                                             "Cannot reset a pending list or one with an active query", this );
    }
    return Underlying->Reset().Inspect( [this] () noexcept
    {
        BoundPipeline.Reset();
        ActiveQueryPool  = nullptr;
        ActiveQueryIndex = 0U;
        DebugDepth       = 0U;
    } );
}

void Lumen::FRHIValidationCommandList::OnAllocatorReset () noexcept
{
    BoundPipeline.Reset();
    ActiveQueryPool  = nullptr;
    ActiveQueryIndex = 0U;
    DebugDepth       = 0U;
}

Lumen::TExpected<void, Lumen::FRHIError>
Lumen::FRHIValidationCommandList::MarkPending ( const FRHITimelineValue InCompletion ) noexcept
{
    if ( GetState() != ERHICommandListState::Executable or not InCompletion.IsValid() )
    {
        return State->GetReporter()->Reject( ERHIValidationMessageCode::InvalidCommandState, ERHIErrorCondition::Command,
                                             "Only an executable list can become pending", this );
    }
    return Underlying->MarkPending( InCompletion );
}

Lumen::TExpected<void, Lumen::FRHIError>
Lumen::FRHIValidationCommandList::MarkCompleted ( const FRHITimelineValue InCompleted ) noexcept
{
    if ( GetState() == ERHICommandListState::Pending and not IsRHICompletionReached( InCompleted, GetCompletion() ) )
    {
        return State->GetReporter()->Reject( ERHIValidationMessageCode::InvalidCommandState, ERHIErrorCondition::Command,
                                             "Command list completion has not been reached", this );
    }
    return Underlying->MarkCompleted( InCompleted );
}

Lumen::TExpected<void, Lumen::FRHIError>
Lumen::FRHIValidationCommandList::Transition ( const TSpan<const FRHITransition> InTransitions ) noexcept
{
    return CheckRecording( true ).AndThen( [this, &InTransitions] () noexcept -> TExpected<void, FRHIError>
    {
        const Bool bTrackStates = State->GetReporter()->GetDesc().IsEnabled( ERHIValidationFlags::ResourceStates );
        TVector<FRHITransition> Unwrapped;
        Unwrapped.reserve( InTransitions.size() );
        for ( const FRHITransition &Transition : InTransitions )
        {
            if ( const FRHIBufferTransition *Buffer = std::get_if<FRHIBufferTransition>( &Transition ) )
            {
                if ( not Buffer->Buffer )
                {
                    return State->GetReporter()->Reject( ERHIValidationMessageCode::InvalidResourceTransition,
                                                         ERHIErrorCondition::Resource, "Buffer transition has no resource",
                                                         this );
                }
                const TExpected<void, FRHIError> Owned = ValidateBuffer( *Buffer->Buffer );
                if ( not Owned )
                {
                    return Owned;
                }
                if ( bTrackStates )
                {
                    const TExpected<void, FRHIError> Valid = State->TransitionBuffer( *Buffer, GetQueueType(), false );
                    if ( not Valid )
                    {
                        return Valid;
                    }
                }
                FRHIBufferTransition Copy = *Buffer;
                Copy.Buffer               = UnwrapRHIShared( Buffer->Buffer );
                Unwrapped.push_back( FRHITransition( Copy ) );
            }
            else if ( const FRHITextureTransition *Texture = std::get_if<FRHITextureTransition>( &Transition ) )
            {
                if ( not Texture->Texture )
                {
                    return State->GetReporter()->Reject( ERHIValidationMessageCode::InvalidResourceTransition,
                                                         ERHIErrorCondition::Resource, "Texture transition has no resource",
                                                         this );
                }
                const TExpected<void, FRHIError> Owned = ValidateTexture( *Texture->Texture );
                if ( not Owned )
                {
                    return Owned;
                }
                if ( bTrackStates )
                {
                    const TExpected<void, FRHIError> Valid = State->TransitionTexture( *Texture, GetQueueType(), false );
                    if ( not Valid )
                    {
                        return Valid;
                    }
                }
                FRHITextureTransition Copy = *Texture;
                Copy.Texture               = UnwrapRHIShared( Texture->Texture );
                Unwrapped.push_back( FRHITransition( Copy ) );
            }
            else
            {
                return State->GetReporter()->Reject( ERHIValidationMessageCode::InvalidResourceTransition,
                                                     ERHIErrorCondition::Resource, "Transition has no active payload", this );
            }
        }
        const TExpected<void, FRHIError> Forwarded = Underlying->Transition( Unwrapped );
        if ( not bTrackStates )
        {
            return Forwarded;
        }
        return Forwarded.AndThen( [this, &InTransitions] () noexcept -> TExpected<void, FRHIError>
        {
            for ( const FRHITransition &Transition : InTransitions )
            {
                if ( const FRHIBufferTransition *Buffer = std::get_if<FRHIBufferTransition>( &Transition ) )
                {
                    const TExpected<void, FRHIError> Applied = State->TransitionBuffer( *Buffer, GetQueueType() );
                    if ( not Applied )
                    {
                        return Applied;
                    }
                }
                else
                {
                    const FRHITextureTransition &Texture     = std::get<FRHITextureTransition>( Transition );
                    const TExpected<void, FRHIError> Applied = State->TransitionTexture( Texture, GetQueueType() );
                    if ( not Applied )
                    {
                        return Applied;
                    }
                }
            }
            return TExpected<void, FRHIError>();
        } );
    } );
}

Lumen::TExpected<void, Lumen::FRHIError> Lumen::FRHIValidationCommandList::CopyBuffer ( TSharedRef<IRHIBuffer> InSource,
                                                                                        TSharedRef<IRHIBuffer> InDestination,
                                                                                        const FRHIBufferCopy &InCopy ) noexcept
{
    const TExpected<void, FRHIError> Scope            = CheckRecording( true );
    const TExpected<void, FRHIError> SourceValid      = ValidateBuffer( *InSource );
    const TExpected<void, FRHIError> DestinationValid = ValidateBuffer( *InDestination );
    if ( not Scope )
    {
        return Scope;
    }
    if ( not SourceValid )
    {
        return SourceValid;
    }
    if ( not DestinationValid )
    {
        return DestinationValid;
    }
    return Underlying->CopyBuffer( UnwrapRHIShared( InSource ), UnwrapRHIShared( InDestination ), InCopy );
}

Lumen::TExpected<void, Lumen::FRHIError> Lumen::FRHIValidationCommandList::CopyTexture ( TSharedRef<IRHITexture> InSource,
                                                                                         TSharedRef<IRHITexture> InDestination,
                                                                                         const FRHITextureCopy &InCopy ) noexcept
{
    const TExpected<void, FRHIError> Scope            = CheckRecording( true );
    const TExpected<void, FRHIError> SourceValid      = ValidateTexture( *InSource );
    const TExpected<void, FRHIError> DestinationValid = ValidateTexture( *InDestination );
    if ( not Scope )
    {
        return Scope;
    }
    if ( not SourceValid )
    {
        return SourceValid;
    }
    if ( not DestinationValid )
    {
        return DestinationValid;
    }
    return Underlying->CopyTexture( UnwrapRHIShared( InSource ), UnwrapRHIShared( InDestination ), InCopy );
}

Lumen::TExpected<void, Lumen::FRHIError> Lumen::FRHIValidationCommandList::CopyBufferToTexture (
    TSharedRef<IRHIBuffer> InSource, TSharedRef<IRHITexture> InDestination, const FRHIBufferTextureCopy &InCopy ) noexcept
{
    const TExpected<void, FRHIError> Scope            = CheckRecording( true );
    const TExpected<void, FRHIError> SourceValid      = ValidateBuffer( *InSource );
    const TExpected<void, FRHIError> DestinationValid = ValidateTexture( *InDestination );
    if ( not Scope )
    {
        return Scope;
    }
    if ( not SourceValid )
    {
        return SourceValid;
    }
    if ( not DestinationValid )
    {
        return DestinationValid;
    }
    return Underlying->CopyBufferToTexture( UnwrapRHIShared( InSource ), UnwrapRHIShared( InDestination ), InCopy );
}

Lumen::TExpected<void, Lumen::FRHIError> Lumen::FRHIValidationCommandList::CopyTextureToBuffer (
    TSharedRef<IRHITexture> InSource, TSharedRef<IRHIBuffer> InDestination, const FRHIBufferTextureCopy &InCopy ) noexcept
{
    const TExpected<void, FRHIError> Scope            = CheckRecording( true );
    const TExpected<void, FRHIError> SourceValid      = ValidateTexture( *InSource );
    const TExpected<void, FRHIError> DestinationValid = ValidateBuffer( *InDestination );
    if ( not Scope )
    {
        return Scope;
    }
    if ( not SourceValid )
    {
        return SourceValid;
    }
    if ( not DestinationValid )
    {
        return DestinationValid;
    }
    return Underlying->CopyTextureToBuffer( UnwrapRHIShared( InSource ), UnwrapRHIShared( InDestination ), InCopy );
}

Lumen::TExpected<void, Lumen::FRHIError>
Lumen::FRHIValidationCommandList::BeginRendering ( const FRHIRenderingDesc &InDesc ) noexcept
{
    const TExpected<void, FRHIError> Scope = CheckRecording( false );
    if ( not Scope )
    {
        return Scope;
    }
    const TExpected<void, FRHIError> Valid = InDesc.Validate();
    if ( not Valid )
    {
        return Valid;
    }
    FRHIRenderingDesc Unwrapped;
    Unwrapped.Area = InDesc.Area;
    Unwrapped.ColorAttachments.reserve( InDesc.ColorAttachments.size() );
    for ( FRHIRenderingAttachment Attachment : InDesc.ColorAttachments )
    {
        const TExpected<void, FRHIError> Owned = State->ValidateObject( *Attachment.View, ERHIValidationObjectKind::TextureView );
        if ( not Owned )
        {
            return Owned;
        }
        Attachment.View = UnwrapRHIShared( Attachment.View );
        Unwrapped.ColorAttachments.push_back( std::move( Attachment ) );
    }
    Unwrapped.DepthStencilAttachment = InDesc.DepthStencilAttachment;
    if ( Unwrapped.DepthStencilAttachment.View )
    {
        const TExpected<void, FRHIError> Owned =
            State->ValidateObject( *Unwrapped.DepthStencilAttachment.View, ERHIValidationObjectKind::TextureView );
        if ( not Owned )
        {
            return Owned;
        }
        Unwrapped.DepthStencilAttachment.View = UnwrapRHIShared( Unwrapped.DepthStencilAttachment.View );
    }
    return Underlying->BeginRendering( Unwrapped );
}

Lumen::TExpected<void, Lumen::FRHIError> Lumen::FRHIValidationCommandList::EndRendering () noexcept
{
    const TExpected<void, FRHIError> Scope = CheckRendering();
    if ( not Scope )
    {
        return Scope;
    }
    return Underlying->EndRendering();
}

Lumen::TExpected<void, Lumen::FRHIError>
Lumen::FRHIValidationCommandList::SetPipeline ( TSharedRef<IRHIPipeline> InPipeline ) noexcept
{
    const TExpected<void, FRHIError> Scope = CheckRecording( true );
    if ( not Scope )
    {
        return Scope;
    }
    if ( ( InPipeline->GetType() == ERHIPipelineType::Graphics ) != ( GetState() == ERHICommandListState::Rendering ) )
    {
        return State->GetReporter()->Reject( ERHIValidationMessageCode::InvalidCommandState, ERHIErrorCondition::Pipeline,
                                             "Pipeline type does not match the current rendering scope", this );
    }
    return Underlying->SetPipeline( InPipeline )
        .Inspect( [this, &InPipeline] () noexcept
    {
        BoundPipeline.Emplace( InPipeline->GetType() );
    } );
}

Lumen::TExpected<void, Lumen::FRHIError>
Lumen::FRHIValidationCommandList::SetBindingSet ( const UInt32 InIndex, TSharedRef<IRHIBindingSet> InSet ) noexcept
{
    return CheckRecording( true )
        .AndThen(
            [this, &InSet] () noexcept -> TExpected<void, FRHIError>
    {
        return State->ValidateObject( *InSet, ERHIValidationObjectKind::BindingSet );
    } )
        .AndThen( [this, InIndex, &InSet] () noexcept -> TExpected<void, FRHIError>
    {
        return Underlying->SetBindingSet( InIndex, UnwrapRHIShared( InSet ) );
    } );
}

Lumen::TExpected<void, Lumen::FRHIError> Lumen::FRHIValidationCommandList::SetVertexBuffer ( const UInt32 InSlot,
                                                                                             TSharedRef<IRHIBuffer> InBuffer,
                                                                                             const UInt64 InOffset ) noexcept
{
    const TExpected<void, FRHIError> Scope = CheckRendering();
    const TExpected<void, FRHIError> Owned = ValidateBuffer( *InBuffer );
    if ( not Scope )
    {
        return Scope;
    }
    if ( not Owned )
    {
        return Owned;
    }
    return Underlying->SetVertexBuffer( InSlot, UnwrapRHIShared( InBuffer ), InOffset );
}

Lumen::TExpected<void, Lumen::FRHIError> Lumen::FRHIValidationCommandList::SetIndexBuffer (
    TSharedRef<IRHIBuffer> InBuffer, const UInt64 InOffset, const ERHIIndexFormat InFormat ) noexcept
{
    const TExpected<void, FRHIError> Scope = CheckRendering();
    const TExpected<void, FRHIError> Owned = ValidateBuffer( *InBuffer );
    if ( not Scope )
    {
        return Scope;
    }
    if ( not Owned )
    {
        return Owned;
    }
    return Underlying->SetIndexBuffer( UnwrapRHIShared( InBuffer ), InOffset, InFormat );
}

Lumen::TExpected<void, Lumen::FRHIError> Lumen::FRHIValidationCommandList::Draw ( const FRHIDrawArguments &InArguments ) noexcept
{
    return CheckDraw( ERHIPipelineType::Graphics, true )
        .AndThen( [this, &InArguments] () noexcept -> TExpected<void, FRHIError>
    {
        return Underlying->Draw( InArguments );
    } );
}

Lumen::TExpected<void, Lumen::FRHIError>
Lumen::FRHIValidationCommandList::DrawIndexed ( const FRHIDrawIndexedArguments &InArguments ) noexcept
{
    return CheckDraw( ERHIPipelineType::Graphics, true )
        .AndThen( [this, &InArguments] () noexcept -> TExpected<void, FRHIError>
    {
        return Underlying->DrawIndexed( InArguments );
    } );
}

Lumen::TExpected<void, Lumen::FRHIError>
Lumen::FRHIValidationCommandList::DrawIndirect ( TSharedRef<IRHIBuffer> InArguments,
                                                 const FRHIIndirectArguments &InDesc ) noexcept
{
    const TExpected<void, FRHIError> Valid = CheckDraw( ERHIPipelineType::Graphics, true );
    const TExpected<void, FRHIError> Owned = ValidateBuffer( *InArguments );
    if ( not Valid )
    {
        return Valid;
    }
    if ( not Owned )
    {
        return Owned;
    }
    return Underlying->DrawIndirect( UnwrapRHIShared( InArguments ), InDesc );
}

Lumen::TExpected<void, Lumen::FRHIError>
Lumen::FRHIValidationCommandList::Dispatch ( const FRHIDispatchArguments &InArguments ) noexcept
{
    return CheckDraw( ERHIPipelineType::Compute, false )
        .AndThen( [this, &InArguments] () noexcept -> TExpected<void, FRHIError>
    {
        return Underlying->Dispatch( InArguments );
    } );
}

Lumen::TExpected<void, Lumen::FRHIError>
Lumen::FRHIValidationCommandList::DispatchIndirect ( TSharedRef<IRHIBuffer> InArguments,
                                                     const FRHIIndirectArguments &InDesc ) noexcept
{
    const TExpected<void, FRHIError> Valid = CheckDraw( ERHIPipelineType::Compute, false );
    const TExpected<void, FRHIError> Owned = ValidateBuffer( *InArguments );
    if ( not Valid )
    {
        return Valid;
    }
    if ( not Owned )
    {
        return Owned;
    }
    return Underlying->DispatchIndirect( UnwrapRHIShared( InArguments ), InDesc );
}

Lumen::TExpected<void, Lumen::FRHIError> Lumen::FRHIValidationCommandList::SetViewport ( const FRHIViewport &InViewport ) noexcept
{
    const TExpected<void, FRHIError> Scope = CheckRendering();
    if ( not Scope )
    {
        return Scope;
    }
    return Underlying->SetViewport( InViewport );
}

Lumen::TExpected<void, Lumen::FRHIError> Lumen::FRHIValidationCommandList::SetScissor ( const FRHIRect2D &InScissor ) noexcept
{
    const TExpected<void, FRHIError> Scope = CheckRendering();
    if ( not Scope )
    {
        return Scope;
    }
    return Underlying->SetScissor( InScissor );
}

Lumen::TExpected<void, Lumen::FRHIError>
Lumen::FRHIValidationCommandList::SetBlendConstants ( const Float32 ( &InConstants )[4] ) noexcept
{
    const TExpected<void, FRHIError> Scope = CheckRendering();
    if ( not Scope )
    {
        return Scope;
    }
    return Underlying->SetBlendConstants( InConstants );
}

Lumen::TExpected<void, Lumen::FRHIError> Lumen::FRHIValidationCommandList::SetDepthBias ( const Float32 InConstant,
                                                                                          const Float32 InSlope ) noexcept
{
    const TExpected<void, FRHIError> Scope = CheckRendering();
    if ( not Scope )
    {
        return Scope;
    }
    return Underlying->SetDepthBias( InConstant, InSlope );
}

Lumen::TExpected<void, Lumen::FRHIError>
Lumen::FRHIValidationCommandList::SetStencilReference ( const UInt32 InReference ) noexcept
{
    const TExpected<void, FRHIError> Scope = CheckRendering();
    if ( not Scope )
    {
        return Scope;
    }
    return Underlying->SetStencilReference( InReference );
}

Lumen::TExpected<void, Lumen::FRHIError> Lumen::FRHIValidationCommandList::BeginQuery ( IRHIQueryPool &InPool,
                                                                                        const UInt32 InIndex ) noexcept
{
    const TExpected<void, FRHIError> Scope = CheckRecording( true );
    if ( not Scope )
    {
        return Scope;
    }
    if ( ActiveQueryPool != nullptr )
    {
        return State->GetReporter()->Reject( ERHIValidationMessageCode::InvalidCommandState, ERHIErrorCondition::Query,
                                             "Another query is already active", this );
    }
    return Underlying->BeginQuery( InPool, InIndex )
        .Inspect( [this, &InPool, InIndex] () noexcept
    {
        ActiveQueryPool  = &InPool;
        ActiveQueryIndex = InIndex;
    } );
}

Lumen::TExpected<void, Lumen::FRHIError> Lumen::FRHIValidationCommandList::EndQuery ( IRHIQueryPool &InPool,
                                                                                      const UInt32 InIndex ) noexcept
{
    const TExpected<void, FRHIError> Scope = CheckRecording( true );
    if ( not Scope )
    {
        return Scope;
    }
    if ( ActiveQueryPool != &InPool or ActiveQueryIndex != InIndex )
    {
        return State->GetReporter()->Reject( ERHIValidationMessageCode::InvalidCommandState, ERHIErrorCondition::Query,
                                             "Ending query does not match the active query", this );
    }
    return Underlying->EndQuery( InPool, InIndex )
        .Inspect( [this] () noexcept
    {
        ActiveQueryPool  = nullptr;
        ActiveQueryIndex = 0U;
    } );
}

Lumen::TExpected<void, Lumen::FRHIError> Lumen::FRHIValidationCommandList::BeginDebugRegion ( const FStringView InName ) noexcept
{
    const TExpected<void, FRHIError> Scope = CheckRecording( true );
    if ( not Scope )
    {
        return Scope;
    }
    return Underlying->BeginDebugRegion( InName ).Inspect( [this] () noexcept
    {
        ++DebugDepth;
    } );
}

Lumen::TExpected<void, Lumen::FRHIError> Lumen::FRHIValidationCommandList::EndDebugRegion () noexcept
{
    const TExpected<void, FRHIError> Scope = CheckRecording( true );
    if ( not Scope )
    {
        return Scope;
    }
    if ( DebugDepth == 0U )
    {
        return State->GetReporter()->Reject( ERHIValidationMessageCode::InvalidCommandState, ERHIErrorCondition::Command,
                                             "No debug region is active", this );
    }
    return Underlying->EndDebugRegion().Inspect( [this] () noexcept
    {
        --DebugDepth;
    } );
}
