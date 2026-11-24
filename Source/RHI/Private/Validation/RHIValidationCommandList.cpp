/**
 * @file RHIValidationCommandList.cpp
 * @brief Implements command recording validation and transparent forwarding.
 */

#include "RHIValidationCommandList.hpp"
#include "RHIValidationCommandAllocator.hpp"

#include <RHI/Pipelines/RHIInlineConstantRange.hpp>
#include <RHI/Queries/IRHIQueryPool.hpp>
#include <RHI/Validation/RHIValidation.hpp>

#include <variant>

/**
 * Public
 */

Lumen::RHI::FRHIValidationCommandList::FRHIValidationCommandList ( TSharedRef<IRHICommandList> InUnderlying,
                                                                   TSharedPtr<FRHIValidationCommandAllocator> InAllocator,
                                                                   TSharedPtr<FRHIValidationState> InState ) noexcept
    : Underlying( std::move( InUnderlying ) ), Allocator( std::move( InAllocator ) ), State( std::move( InState ) )
{
    State->Track( *Underlying, ERHIValidationObjectKind::CommandList );
    Allocator->RegisterList( *this );
}

Lumen::RHI::FRHIValidationCommandList::~FRHIValidationCommandList () noexcept
{
    Allocator->UnregisterList( *this );
    State->Untrack( *Underlying );
}

LUMEN_RHI_VALIDATION_OBJECT_FORWARDERS( FRHIValidationCommandList )

Lumen::RHI::ERHIQueueType Lumen::RHI::FRHIValidationCommandList::GetQueueType () const noexcept
{
    return Underlying->GetQueueType();
}

Lumen::RHI::ERHICommandListState Lumen::RHI::FRHIValidationCommandList::GetState () const noexcept
{
    return Underlying->GetState();
}

Lumen::RHI::FRHITimelineValue Lumen::RHI::FRHIValidationCommandList::GetCompletion () const noexcept
{
    return Underlying->GetCompletion();
}

const Lumen::RHI::FRHICommandStream &Lumen::RHI::FRHIValidationCommandList::GetStream () const noexcept
{
    return Underlying->GetStream();
}

Lumen::RHI::FRHIExpected Lumen::RHI::FRHIValidationCommandList::Begin () noexcept
{
    if ( IsCommandValidationEnabled() and GetState() != ERHICommandListState::Initial )
    {
        return State->GetReporter()->Reject( ERHIValidationMessageCode::InvalidCommandState, ERHIErrorCondition::Command,
                                             "Only an initial command list can begin", this );
    }
    return Underlying->Begin();
}

Lumen::RHI::FRHIExpected Lumen::RHI::FRHIValidationCommandList::End () noexcept
{
    if ( IsCommandValidationEnabled() and
         ( GetState() != ERHICommandListState::Recording or ActiveQueryPool != nullptr or DebugDepth != 0U ) )
    {
        return State->GetReporter()->Reject( ERHIValidationMessageCode::InvalidCommandState, ERHIErrorCondition::Command,
                                             "Command list has an open rendering, query, or debug scope", this );
    }
    return Underlying->End();
}

Lumen::RHI::FRHIExpected Lumen::RHI::FRHIValidationCommandList::Reset () noexcept
{
    if ( IsCommandValidationEnabled() and ( GetState() == ERHICommandListState::Pending or ActiveQueryPool != nullptr ) )
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

void Lumen::RHI::FRHIValidationCommandList::OnAllocatorReset () noexcept
{
    BoundPipeline.Reset();
    ActiveQueryPool  = nullptr;
    ActiveQueryIndex = 0U;
    DebugDepth       = 0U;
}

Lumen::RHI::FRHIExpected Lumen::RHI::FRHIValidationCommandList::MarkPending ( const FRHITimelineValue InCompletion ) noexcept
{
    if ( IsCommandValidationEnabled() and ( GetState() != ERHICommandListState::Executable or not InCompletion.IsValid() ) )
    {
        return State->GetReporter()->Reject( ERHIValidationMessageCode::InvalidCommandState, ERHIErrorCondition::Command,
                                             "Only an executable list can become pending", this );
    }
    return Underlying->MarkPending( InCompletion );
}

Lumen::RHI::FRHIExpected Lumen::RHI::FRHIValidationCommandList::MarkCompleted ( const FRHITimelineValue InCompleted ) noexcept
{
    if ( IsCommandValidationEnabled() and GetState() == ERHICommandListState::Pending and
         not IsRHICompletionReached( InCompleted, GetCompletion() ) )
    {
        return State->GetReporter()->Reject( ERHIValidationMessageCode::InvalidCommandState, ERHIErrorCondition::Command,
                                             "Command list completion has not been reached", this );
    }
    return Underlying->MarkCompleted( InCompleted );
}

Lumen::RHI::FRHIExpected
Lumen::RHI::FRHIValidationCommandList::Transition ( const TSpan<const FRHITransition> InTransitions ) noexcept
{
    return CheckRecording( true ).AndThen( [this, &InTransitions] () noexcept -> FRHIExpected
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
                LUMEN_TRY_EXPECT_ERROR( ValidateBuffer( *Buffer->Buffer ) );
                FRHIBufferTransition Copy = *Buffer;
                Copy.Buffer               = UnwrapRHIShared( Buffer->Buffer );
                Unwrapped.emplace_back( Copy );
            }
            else if ( const FRHITextureTransition *Texture = std::get_if<FRHITextureTransition>( &Transition ) )
            {
                if ( not Texture->Texture )
                {
                    return State->GetReporter()->Reject( ERHIValidationMessageCode::InvalidResourceTransition,
                                                         ERHIErrorCondition::Resource, "Texture transition has no resource",
                                                         this );
                }
                LUMEN_TRY_EXPECT_ERROR( ValidateTexture( *Texture->Texture ) );
                FRHITextureTransition Copy = *Texture;
                Copy.Texture               = UnwrapRHIShared( Texture->Texture );
                Unwrapped.emplace_back( Copy );
            }
            else
            {
                return State->GetReporter()->Reject( ERHIValidationMessageCode::InvalidResourceTransition,
                                                     ERHIErrorCondition::Resource, "Transition has no active payload", this );
            }
        }
        if ( bTrackStates )
        {
            LUMEN_TRY_EXPECT_ERROR( State->ValidateTransitions( InTransitions, GetQueueType() ) );
        }
        FRHIExpected Forwarded = Underlying->Transition( Unwrapped );
        if ( bTrackStates and Forwarded )
        {
            State->CommitTransitions( InTransitions );
        }
        return Forwarded;
    } );
}

Lumen::RHI::FRHIExpected Lumen::RHI::FRHIValidationCommandList::CopyBuffer ( TSharedRef<IRHIBuffer> InSource,
                                                                             TSharedRef<IRHIBuffer> InDestination,
                                                                             const FRHIBufferCopy &InCopy ) noexcept
{
    LUMEN_TRY_EXPECT_ERROR( CheckRecording( true ) );
    LUMEN_TRY_EXPECT_ERROR( ValidateBuffer( *InSource ) );
    LUMEN_TRY_EXPECT_ERROR( ValidateBuffer( *InDestination ) );

    return Underlying->CopyBuffer( UnwrapRHIShared( InSource ), UnwrapRHIShared( InDestination ), InCopy );
}

Lumen::RHI::FRHIExpected Lumen::RHI::FRHIValidationCommandList::CopyTexture ( TSharedRef<IRHITexture> InSource,
                                                                              TSharedRef<IRHITexture> InDestination,
                                                                              const FRHITextureCopy &InCopy ) noexcept
{
    LUMEN_TRY_EXPECT_ERROR( CheckRecording( true ) );
    LUMEN_TRY_EXPECT_ERROR( ValidateTexture( *InSource ) );
    LUMEN_TRY_EXPECT_ERROR( ValidateTexture( *InDestination ) );

    return Underlying->CopyTexture( UnwrapRHIShared( InSource ), UnwrapRHIShared( InDestination ), InCopy );
}

Lumen::RHI::FRHIExpected Lumen::RHI::FRHIValidationCommandList::CopyBufferToTexture (
    TSharedRef<IRHIBuffer> InSource, TSharedRef<IRHITexture> InDestination, const FRHIBufferTextureCopy &InCopy ) noexcept
{
    LUMEN_TRY_EXPECT_ERROR( CheckRecording( true ) );
    LUMEN_TRY_EXPECT_ERROR( ValidateBuffer( *InSource ) );
    LUMEN_TRY_EXPECT_ERROR( ValidateTexture( *InDestination ) );

    return Underlying->CopyBufferToTexture( UnwrapRHIShared( InSource ), UnwrapRHIShared( InDestination ), InCopy );
}

Lumen::RHI::FRHIExpected Lumen::RHI::FRHIValidationCommandList::CopyTextureToBuffer (
    TSharedRef<IRHITexture> InSource, TSharedRef<IRHIBuffer> InDestination, const FRHIBufferTextureCopy &InCopy ) noexcept
{
    LUMEN_TRY_EXPECT_ERROR( CheckRecording( true ) );
    LUMEN_TRY_EXPECT_ERROR( ValidateTexture( *InSource ) );
    LUMEN_TRY_EXPECT_ERROR( ValidateBuffer( *InDestination ) );

    return Underlying->CopyTextureToBuffer( UnwrapRHIShared( InSource ), UnwrapRHIShared( InDestination ), InCopy );
}

Lumen::RHI::FRHIExpected Lumen::RHI::FRHIValidationCommandList::BeginRendering ( const FRHIRenderingDesc &InDesc ) noexcept
{
    LUMEN_TRY_EXPECT_ERROR( CheckRecording( false ) );
    LUMEN_TRY_EXPECT_ERROR( InDesc.Validate() );

    FRHIRenderingDesc Unwrapped;
    Unwrapped.Area = InDesc.Area;
    Unwrapped.ColorAttachments.reserve( InDesc.ColorAttachments.size() );
    for ( FRHIRenderingAttachment Attachment : InDesc.ColorAttachments )
    {
        LUMEN_TRY_EXPECT_ERROR( State->ValidateObject( *Attachment.View, ERHIValidationObjectKind::TextureView ) );
        Attachment.View = UnwrapRHIShared( Attachment.View );
        Unwrapped.ColorAttachments.push_back( std::move( Attachment ) );
    }
    Unwrapped.DepthStencilAttachment = InDesc.DepthStencilAttachment;
    if ( Unwrapped.DepthStencilAttachment.View )
    {
        LUMEN_TRY_EXPECT_ERROR(
            State->ValidateObject( *Unwrapped.DepthStencilAttachment.View, ERHIValidationObjectKind::TextureView ) );
        Unwrapped.DepthStencilAttachment.View = UnwrapRHIShared( Unwrapped.DepthStencilAttachment.View );
    }
    return Underlying->BeginRendering( Unwrapped );
}

Lumen::RHI::FRHIExpected Lumen::RHI::FRHIValidationCommandList::EndRendering () noexcept
{
    LUMEN_TRY_EXPECT_ERROR( CheckRendering() );

    return Underlying->EndRendering();
}

Lumen::RHI::FRHIExpected Lumen::RHI::FRHIValidationCommandList::SetPipeline ( TSharedRef<IRHIPipeline> InPipeline ) noexcept
{
    LUMEN_TRY_EXPECT_ERROR( CheckRecording( true ) );

    if ( IsCommandValidationEnabled() and
         ( ( InPipeline->GetType() == ERHIPipelineType::Graphics ) != ( GetState() == ERHICommandListState::Rendering ) ) )
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

Lumen::RHI::FRHIExpected Lumen::RHI::FRHIValidationCommandList::SetBindingSet ( const UInt32 InIndex,
                                                                                TSharedRef<IRHIBindingSet> InSet ) noexcept
{
    return CheckRecording( true )
        .AndThen(
            [this, &InSet] () noexcept -> FRHIExpected
    {
        return State->ValidateObject( *InSet, ERHIValidationObjectKind::BindingSet );
    } )
        .AndThen( [this, InIndex, &InSet] () noexcept -> FRHIExpected
    {
        return Underlying->SetBindingSet( InIndex, UnwrapRHIShared( InSet ) );
    } );
}

Lumen::RHI::FRHIExpected Lumen::RHI::FRHIValidationCommandList::SetInlineConstants ( const ERHIShaderStage InStages,
                                                                                     const UInt32 InOffset,
                                                                                     const TSpan<const Byte> InValues ) noexcept
{
    LUMEN_TRY_EXPECT_ERROR( CheckRecording( true ) );

    if ( IsCommandValidationEnabled() and not BoundPipeline )
    {
        return State->GetReporter()->Reject( ERHIValidationMessageCode::InvalidCommandState, ERHIErrorCondition::Pipeline,
                                             "Inline constants require a bound pipeline", this );
    }

    LUMEN_TRY_EXPECT_ERROR( ValidateRHIInlineConstantWrite( InStages, InOffset, InValues.size() ) );

    return Underlying->SetInlineConstants( InStages, InOffset, InValues );
}

Lumen::RHI::FRHIExpected Lumen::RHI::FRHIValidationCommandList::SetVertexBuffer ( const UInt32 InSlot,
                                                                                  TSharedRef<IRHIBuffer> InBuffer,
                                                                                  const UInt64 InOffset ) noexcept
{
    LUMEN_TRY_EXPECT_ERROR( CheckRendering() );
    LUMEN_TRY_EXPECT_ERROR( ValidateBuffer( *InBuffer ) );

    return Underlying->SetVertexBuffer( InSlot, UnwrapRHIShared( InBuffer ), InOffset );
}

Lumen::RHI::FRHIExpected Lumen::RHI::FRHIValidationCommandList::SetIndexBuffer ( TSharedRef<IRHIBuffer> InBuffer,
                                                                                 const UInt64 InOffset,
                                                                                 const ERHIIndexFormat InFormat ) noexcept
{
    LUMEN_TRY_EXPECT_ERROR( CheckRendering() );
    LUMEN_TRY_EXPECT_ERROR( ValidateBuffer( *InBuffer ) );

    return Underlying->SetIndexBuffer( UnwrapRHIShared( InBuffer ), InOffset, InFormat );
}

Lumen::RHI::FRHIExpected Lumen::RHI::FRHIValidationCommandList::Draw ( const FRHIDrawArguments &InArguments ) noexcept
{
    return CheckDraw( ERHIPipelineType::Graphics, true )
        .AndThen( [this, &InArguments] () noexcept -> FRHIExpected
    {
        return Underlying->Draw( InArguments );
    } );
}

Lumen::RHI::FRHIExpected
Lumen::RHI::FRHIValidationCommandList::DrawIndexed ( const FRHIDrawIndexedArguments &InArguments ) noexcept
{
    return CheckDraw( ERHIPipelineType::Graphics, true )
        .AndThen( [this, &InArguments] () noexcept -> FRHIExpected
    {
        return Underlying->DrawIndexed( InArguments );
    } );
}

Lumen::RHI::FRHIExpected Lumen::RHI::FRHIValidationCommandList::DrawIndirect ( TSharedRef<IRHIBuffer> InArguments,
                                                                               const FRHIIndirectArguments &InDesc ) noexcept
{
    LUMEN_TRY_EXPECT_ERROR( CheckDraw( ERHIPipelineType::Graphics, true ) );
    LUMEN_TRY_EXPECT_ERROR( ValidateBuffer( *InArguments ) );

    return Underlying->DrawIndirect( UnwrapRHIShared( InArguments ), InDesc );
}

Lumen::RHI::FRHIExpected Lumen::RHI::FRHIValidationCommandList::Dispatch ( const FRHIDispatchArguments &InArguments ) noexcept
{
    return CheckDraw( ERHIPipelineType::Compute, false )
        .AndThen( [this, &InArguments] () noexcept -> FRHIExpected
    {
        return Underlying->Dispatch( InArguments );
    } );
}

Lumen::RHI::FRHIExpected Lumen::RHI::FRHIValidationCommandList::DispatchIndirect ( TSharedRef<IRHIBuffer> InArguments,
                                                                                   const FRHIIndirectArguments &InDesc ) noexcept
{
    LUMEN_TRY_EXPECT_ERROR( CheckDraw( ERHIPipelineType::Compute, false ) );
    LUMEN_TRY_EXPECT_ERROR( ValidateBuffer( *InArguments ) );

    return Underlying->DispatchIndirect( UnwrapRHIShared( InArguments ), InDesc );
}

Lumen::RHI::FRHIExpected Lumen::RHI::FRHIValidationCommandList::SetViewport ( const FRHIViewport &InViewport ) noexcept
{
    LUMEN_TRY_EXPECT_ERROR( CheckRendering() );

    return Underlying->SetViewport( InViewport );
}

Lumen::RHI::FRHIExpected Lumen::RHI::FRHIValidationCommandList::SetScissor ( const FRHIRect2D &InScissor ) noexcept
{
    LUMEN_TRY_EXPECT_ERROR( CheckRendering() );

    return Underlying->SetScissor( InScissor );
}

Lumen::RHI::FRHIExpected Lumen::RHI::FRHIValidationCommandList::SetBlendConstants ( const Float32 ( &InConstants )[4] ) noexcept
{
    LUMEN_TRY_EXPECT_ERROR( CheckRendering() );

    return Underlying->SetBlendConstants( InConstants );
}

Lumen::RHI::FRHIExpected Lumen::RHI::FRHIValidationCommandList::SetDepthBias ( const Float32 InConstant,
                                                                               const Float32 InSlope ) noexcept
{
    LUMEN_TRY_EXPECT_ERROR( CheckRendering() );

    return Underlying->SetDepthBias( InConstant, InSlope );
}

Lumen::RHI::FRHIExpected Lumen::RHI::FRHIValidationCommandList::SetStencilReference ( const UInt32 InReference ) noexcept
{
    LUMEN_TRY_EXPECT_ERROR( CheckRendering() );

    return Underlying->SetStencilReference( InReference );
}

Lumen::RHI::FRHIExpected Lumen::RHI::FRHIValidationCommandList::BeginQuery ( TSharedRef<IRHIQueryPool> InPool,
                                                                             const UInt32 InIndex ) noexcept
{
    LUMEN_TRY_EXPECT_ERROR( CheckRecording( true ) );

    if ( IsCommandValidationEnabled() and ActiveQueryPool != nullptr )
    {
        return State->GetReporter()->Reject( ERHIValidationMessageCode::InvalidCommandState, ERHIErrorCondition::Query,
                                             "Another query is already active", this );
    }
    return Underlying->BeginQuery( InPool, InIndex )
        .Inspect( [this, &InPool, InIndex] () noexcept
    {
        ActiveQueryPool  = InPool.Get();
        ActiveQueryIndex = InIndex;
    } );
}

Lumen::RHI::FRHIExpected Lumen::RHI::FRHIValidationCommandList::EndQuery ( TSharedRef<IRHIQueryPool> InPool,
                                                                           const UInt32 InIndex ) noexcept
{
    LUMEN_TRY_EXPECT_ERROR( CheckRecording( true ) );

    if ( IsCommandValidationEnabled() and ( ActiveQueryPool != InPool.Get() or ActiveQueryIndex != InIndex ) )
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

Lumen::RHI::FRHIExpected Lumen::RHI::FRHIValidationCommandList::BeginDebugRegion ( const FStringView InName ) noexcept
{
    LUMEN_TRY_EXPECT_ERROR( CheckRecording( true ) );

    return Underlying->BeginDebugRegion( InName ).Inspect( [this] () noexcept
    {
        ++DebugDepth;
    } );
}

Lumen::RHI::FRHIExpected Lumen::RHI::FRHIValidationCommandList::EndDebugRegion () noexcept
{
    LUMEN_TRY_EXPECT_ERROR( CheckRecording( true ) );

    if ( IsCommandValidationEnabled() and DebugDepth == 0U )
    {
        return State->GetReporter()->Reject( ERHIValidationMessageCode::InvalidCommandState, ERHIErrorCondition::Command,
                                             "No debug region is active", this );
    }
    return Underlying->EndDebugRegion().Inspect( [this] () noexcept
    {
        --DebugDepth;
    } );
}

/**
 * Private
 */

Lumen::Bool Lumen::RHI::FRHIValidationCommandList::IsCommandValidationEnabled () const noexcept
{
    return State->GetReporter()->GetDesc().IsEnabled( ERHIValidationFlags::Commands );
}

Lumen::RHI::FRHIExpected Lumen::RHI::FRHIValidationCommandList::CheckRecording ( const Bool bAllowRendering ) const noexcept
{
    if ( not IsCommandValidationEnabled() )
    {
        return {};
    }
    const ERHICommandListState Current = GetState();
    if ( Current == ERHICommandListState::Recording or ( bAllowRendering and Current == ERHICommandListState::Rendering ) )
    {
        return {};
    }
    return State->GetReporter()->Reject( ERHIValidationMessageCode::InvalidCommandState, ERHIErrorCondition::Command,
                                         "Command is outside a compatible recording scope", this );
}

Lumen::RHI::FRHIExpected Lumen::RHI::FRHIValidationCommandList::CheckRendering () const noexcept
{
    if ( not IsCommandValidationEnabled() )
    {
        return {};
    }
    if ( GetState() == ERHICommandListState::Rendering )
    {
        return {};
    }
    return State->GetReporter()->Reject( ERHIValidationMessageCode::InvalidCommandState, ERHIErrorCondition::Command,
                                         "Command requires an active rendering scope", this );
}

Lumen::RHI::FRHIExpected Lumen::RHI::FRHIValidationCommandList::ValidateBuffer ( const IRHIBuffer &InBuffer ) const noexcept
{
    return State->ValidateObject( InBuffer, ERHIValidationObjectKind::Buffer );
}

Lumen::RHI::FRHIExpected Lumen::RHI::FRHIValidationCommandList::ValidateTexture ( const IRHITexture &InTexture ) const noexcept
{
    return State->ValidateObject( InTexture, ERHIValidationObjectKind::Texture );
}

Lumen::RHI::FRHIExpected Lumen::RHI::FRHIValidationCommandList::CheckDraw ( const ERHIPipelineType InType,
                                                                            const Bool bRendering ) const noexcept
{
    if ( not IsCommandValidationEnabled() )
    {
        return {};
    }
    return ( bRendering ? CheckRendering() : CheckRecording( false ) )
        .AndThen( [this, InType] () noexcept -> FRHIExpected
    {
        if ( not BoundPipeline or *BoundPipeline != InType )
        {
            return State->GetReporter()->Reject( ERHIValidationMessageCode::InvalidCommandState, ERHIErrorCondition::Pipeline,
                                                 "Command has no compatible bound pipeline", this );
        }
        return {};
    } );
}
