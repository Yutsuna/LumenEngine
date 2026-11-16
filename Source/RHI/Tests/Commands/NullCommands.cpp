/**
 * @file NullCommands.cpp
 * @brief Implements deterministic Null command recording.
 */

#include "Commands/NullCommands.hpp"
#include <Templates/Hash.hpp>
#include <cmath>
#include <variant>

namespace Lumen
{

namespace
{
    TExpected<void, FRHIError> StateError ( FStringView InMessage ) noexcept
    {
        return TUnexpected<FRHIError>( FRHIError( ERHIErrorCode::InvalidState, ERHIErrorCondition::Command, InMessage ) );
    }
    Bool HasUsage ( const ERHIResourceUsage InUsage, const ERHIResourceUsage InRequired ) noexcept
    {
        return HasAnyFlags( InUsage, InRequired );
    }
    Bool IsTextureRegionValid ( const IRHITexture &InTexture,
                                const UInt32 InMip,
                                const UInt32 InLayer,
                                const FRHIOffset3D &InOffset,
                                const FRHIExtent3D &InExtent ) noexcept
    {
        const auto &Desc = InTexture.GetDesc();
        if ( InMip >= Desc.MipCount or InLayer >= Desc.LayerCount or not InOffset.IsNonNegative() )
        {
            return false;
        }
        const UInt32 Width  = Desc.Extent.Width > InMip ? Desc.Extent.Width >> InMip : 1U;
        const UInt32 Height = Desc.Extent.Height > InMip ? Desc.Extent.Height >> InMip : 1U;
        const UInt32 Depth  = Desc.Extent.Depth > InMip ? Desc.Extent.Depth >> InMip : 1U;
        return static_cast<UInt64>( InOffset.X ) + InExtent.Width <= Width and
               static_cast<UInt64>( InOffset.Y ) + InExtent.Height <= Height and
               static_cast<UInt64>( InOffset.Z ) + InExtent.Depth <= Depth;
    }

} // namespace

/**
 * Public
 */

Lumen::Tests::FNullCommandAllocator::FNullCommandAllocator ( const ERHIQueueType InQueueType, const FRHIObjectId InId ) noexcept
    : FRHICommandAllocatorBase( InQueueType ), Identity( InId, FHash::TypeHash<FNullCommandAllocator>() )
{
    /* */
}

LUMEN_RHI_OBJECT_FORWARDERS( Tests::FNullCommandAllocator )
Lumen::Tests::FNullCommandList::FNullCommandList ( TSharedPtr<FNullCommandAllocator> InAllocator,
                                                   const FRHIObjectId InId ) noexcept
    : Identity( InId, FHash::TypeHash<FNullCommandList>() ), Allocator( std::move( InAllocator ) )
{
    Allocator->RegisterList( *this );
}

Lumen::Tests::FNullCommandList::~FNullCommandList () noexcept
{
    Allocator->UnregisterList( *this );
}

LUMEN_RHI_OBJECT_FORWARDERS( Tests::FNullCommandList )
Lumen::ERHIQueueType Lumen::Tests::FNullCommandList::GetQueueType () const noexcept
{
    return Allocator->GetQueueType();
}

Lumen::ERHICommandListState Lumen::Tests::FNullCommandList::GetState () const noexcept
{
    return State;
}

Lumen::FRHITimelineValue Lumen::Tests::FNullCommandList::GetCompletion () const noexcept
{
    return Completion;
}

const Lumen::FRHICommandStream &Lumen::Tests::FNullCommandList::GetStream () const noexcept
{
    return Stream;
}

Lumen::TExpected<void, Lumen::FRHIError> Lumen::Tests::FNullCommandList::Begin () noexcept
{
    return RHITransitionCommandList( State, ERHICommandListState::Recording );
}

Lumen::TExpected<void, Lumen::FRHIError> Lumen::Tests::FNullCommandList::End () noexcept
{
    return RHITransitionCommandList( State, ERHICommandListState::Executable );
}

Lumen::TExpected<void, Lumen::FRHIError> Lumen::Tests::FNullCommandList::Reset () noexcept
{
    if ( State == ERHICommandListState::Pending )
    {
        return StateError( "Cannot reset a pending Null command list" );
    }
    if ( State != ERHICommandListState::Initial )
    {
        auto Result = RHITransitionCommandList( State, ERHICommandListState::Initial );
        if ( not Result )
        {
            return Result;
        }
    }
    Stream.Clear();
    Completion = {};
    BoundPipeline.Reset();
    DebugDepth = 0U;
    return {};
}

Lumen::TExpected<void, Lumen::FRHIError>
Lumen::Tests::FNullCommandList::MarkPending ( const FRHITimelineValue InCompletion ) noexcept
{
    if ( not InCompletion.IsValid() )
    {
        return TUnexpected<FRHIError>( MakeRHIInvalidArgument( ERHIErrorCondition::Command, "Pending completion is zero" ) );
    }
    auto Result = RHITransitionCommandList( State, ERHICommandListState::Pending );
    if ( Result )
    {
        Completion = InCompletion;
    }
    return Result;
}

Lumen::TExpected<void, Lumen::FRHIError>
Lumen::Tests::FNullCommandList::MarkCompleted ( const FRHITimelineValue InCompleted ) noexcept
{
    if ( State == ERHICommandListState::Pending and not IsRHICompletionReached( InCompleted, Completion ) )
    {
        return StateError( "Command list completion has not been reached" );
    }
    auto Result = RHITransitionCommandList( State, ERHICommandListState::Executable );
    if ( Result )
    {
        Completion = {};
    }
    return Result;
}

Lumen::TExpected<void, Lumen::FRHIError>
Lumen::Tests::FNullCommandList::CheckRecording ( const Bool bAllowRendering ) const noexcept
{
    if ( State != ERHICommandListState::Recording and ( not bAllowRendering or State != ERHICommandListState::Rendering ) )
    {
        return StateError( "Command is outside a recording scope" );
    }
    return {};
}

Lumen::TExpected<void, Lumen::FRHIError> Lumen::Tests::FNullCommandList::CheckRendering () const noexcept
{
    if ( State != ERHICommandListState::Rendering )
    {
        return StateError( "Command requires an active rendering scope" );
    }
    return {};
}

Lumen::TExpected<void, Lumen::FRHIError>
Lumen::Tests::FNullCommandList::Transition ( const TSpan<const FRHITransition> InTransitions ) noexcept
{
    auto StateValid = CheckRecording( true );
    if ( not StateValid )
    {
        return StateValid;
    }
    for ( const auto &Transition : InTransitions )
    {
        auto Valid = ValidateRHITransition( Transition );
        if ( not Valid )
        {
            return Valid;
        }
    }
    for ( const auto &Transition : InTransitions )
    {
        struct FRecord
        {
            ERHIResourceState Before;
            ERHIResourceState After;
        } Record{};
        if ( const auto *Buffer = std::get_if<FRHIBufferTransition>( &Transition ) )
        {
            Record = { .Before = Buffer->Before, .After = Buffer->After };
            Stream.Retain( StaticCastSharedPtr<IRHIObject>( Buffer->Buffer ) );
        }
        else if ( const auto *Texture = std::get_if<FRHITextureTransition>( &Transition ) )
        {
            Record = { .Before = Texture->Before, .After = Texture->After };
            Stream.Retain( StaticCastSharedPtr<IRHIObject>( Texture->Texture ) );
        }
        else
        {
            return TUnexpected<FRHIError>( FRHIError( ERHIErrorCode::CorruptedData, ERHIErrorCondition::Command,
                                                      "Transition variant has no active payload" ) );
        }
        auto Result = Stream.Write( static_cast<UInt32>( ERHINullCommandOpcode::Transition ), Record );
        if ( not Result )
        {
            return Result;
        }
    }
    return {};
}

Lumen::TExpected<void, Lumen::FRHIError> Lumen::Tests::FNullCommandList::CopyBuffer ( TSharedRef<IRHIBuffer> InSource,
                                                                                      TSharedRef<IRHIBuffer> InDestination,
                                                                                      const FRHIBufferCopy &InCopy ) noexcept
{
    auto StateValid = CheckRecording( true );
    auto Valid =
        InCopy.Validate( InSource->GetDesc().Size, InDestination->GetDesc().Size, InSource.Get() == InDestination.Get() );
    if ( not StateValid )
    {
        return StateValid;
    }
    if ( not Valid or not HasUsage( InSource->GetDesc().Usage, ERHIResourceUsage::CopySource ) or
         not HasUsage( InDestination->GetDesc().Usage, ERHIResourceUsage::CopyDestination ) )
    {
        return TUnexpected<FRHIError>( MakeRHIInvalidArgument( ERHIErrorCondition::Command, "Invalid buffer copy or usage" ) );
    }
    auto Result = Stream.Write( static_cast<UInt32>( ERHINullCommandOpcode::CopyBuffer ), InCopy );
    if ( Result )
    {
        Stream.Retain( InSource );
        Stream.Retain( InDestination );
    }
    return Result;
}

Lumen::TExpected<void, Lumen::FRHIError> Lumen::Tests::FNullCommandList::CopyTexture ( TSharedRef<IRHITexture> InSource,
                                                                                       TSharedRef<IRHITexture> InDestination,
                                                                                       const FRHITextureCopy &InCopy ) noexcept
{
    auto StateValid = CheckRecording( true );
    auto Valid      = InCopy.Validate( InSource->GetDesc().Format );
    if ( not StateValid )
    {
        return StateValid;
    }
    if ( not Valid or InSource->GetDesc().Format != InDestination->GetDesc().Format or
         not IsTextureRegionValid( *InSource, InCopy.SourceMip, InCopy.SourceLayer, InCopy.SourceOffset, InCopy.Extent ) or
         not IsTextureRegionValid( *InDestination, InCopy.DestinationMip, InCopy.DestinationLayer, InCopy.DestinationOffset,
                                   InCopy.Extent ) or
         not HasUsage( InSource->GetDesc().Usage, ERHIResourceUsage::CopySource ) or
         not HasUsage( InDestination->GetDesc().Usage, ERHIResourceUsage::CopyDestination ) )
    {
        return TUnexpected<FRHIError>( MakeRHIInvalidArgument( ERHIErrorCondition::Command, "Invalid texture copy or usage" ) );
    }
    auto Result = Stream.Write( static_cast<UInt32>( ERHINullCommandOpcode::CopyTexture ), InCopy );
    if ( Result )
    {
        Stream.Retain( InSource );
        Stream.Retain( InDestination );
    }
    return Result;
}

Lumen::TExpected<void, Lumen::FRHIError> Lumen::Tests::FNullCommandList::CopyBufferToTexture (
    TSharedRef<IRHIBuffer> InSource, TSharedRef<IRHITexture> InDestination, const FRHIBufferTextureCopy &InCopy ) noexcept
{
    auto StateValid = CheckRecording( true );
    auto Valid      = InCopy.Validate( InDestination->GetDesc().Format );
    if ( not StateValid )
    {
        return StateValid;
    }
    if ( not Valid or not IsTextureRegionValid( *InDestination, InCopy.Mip, InCopy.Layer, InCopy.TextureOffset, InCopy.Extent ) or
         not HasUsage( InSource->GetDesc().Usage, ERHIResourceUsage::CopySource ) or
         not HasUsage( InDestination->GetDesc().Usage, ERHIResourceUsage::CopyDestination ) )
    {
        return TUnexpected<FRHIError>( MakeRHIInvalidArgument( ERHIErrorCondition::Command, "Invalid buffer-to-texture copy" ) );
    }
    auto Result = Stream.Write( static_cast<UInt32>( ERHINullCommandOpcode::CopyBufferToTexture ), InCopy );
    if ( Result )
    {
        Stream.Retain( InSource );
        Stream.Retain( InDestination );
    }
    return Result;
}

Lumen::TExpected<void, Lumen::FRHIError> Lumen::Tests::FNullCommandList::CopyTextureToBuffer (
    TSharedRef<IRHITexture> InSource, TSharedRef<IRHIBuffer> InDestination, const FRHIBufferTextureCopy &InCopy ) noexcept
{
    auto StateValid = CheckRecording( true );
    auto Valid      = InCopy.Validate( InSource->GetDesc().Format );
    if ( not StateValid )
    {
        return StateValid;
    }
    if ( not Valid or not IsTextureRegionValid( *InSource, InCopy.Mip, InCopy.Layer, InCopy.TextureOffset, InCopy.Extent ) or
         not HasUsage( InSource->GetDesc().Usage, ERHIResourceUsage::CopySource ) or
         not HasUsage( InDestination->GetDesc().Usage, ERHIResourceUsage::CopyDestination ) )
    {
        return TUnexpected<FRHIError>( MakeRHIInvalidArgument( ERHIErrorCondition::Command, "Invalid texture-to-buffer copy" ) );
    }
    auto Result = Stream.Write( static_cast<UInt32>( ERHINullCommandOpcode::CopyTextureToBuffer ), InCopy );
    if ( Result )
    {
        Stream.Retain( InSource );
        Stream.Retain( InDestination );
    }
    return Result;
}

Lumen::TExpected<void, Lumen::FRHIError>
Lumen::Tests::FNullCommandList::BeginRendering ( const FRHIRenderingDesc &InDesc ) noexcept
{
    if ( GetQueueType() != ERHIQueueType::Graphics )
    {
        return StateError( "Rendering requires a graphics command list" );
    }
    auto StateValid = CheckRecording( false );
    auto Valid      = InDesc.Validate();
    if ( not StateValid )
    {
        return StateValid;
    }
    if ( not Valid )
    {
        return Valid;
    }
    auto Result = Stream.Write( static_cast<UInt32>( ERHINullCommandOpcode::BeginRendering ), InDesc.Area );
    if ( not Result )
    {
        return Result;
    }
    for ( const auto &Attachment : InDesc.ColorAttachments )
    {
        Stream.Retain( StaticCastSharedPtr<IRHIObject>( Attachment.View ) );
    }
    if ( InDesc.DepthStencilAttachment.View )
    {
        Stream.Retain( StaticCastSharedPtr<IRHIObject>( InDesc.DepthStencilAttachment.View ) );
    }
    return RHITransitionCommandList( State, ERHICommandListState::Rendering );
}

Lumen::TExpected<void, Lumen::FRHIError> Lumen::Tests::FNullCommandList::EndRendering () noexcept
{
    auto StateValid = CheckRendering();
    if ( not StateValid )
    {
        return StateValid;
    }
    auto Result = Stream.WritePayload( static_cast<UInt32>( ERHINullCommandOpcode::EndRendering ), {} );
    if ( not Result )
    {
        return Result;
    }
    return RHITransitionCommandList( State, ERHICommandListState::Recording );
}

Lumen::TExpected<void, Lumen::FRHIError>
Lumen::Tests::FNullCommandList::SetPipeline ( TSharedRef<IRHIPipeline> InPipeline ) noexcept
{
    auto StateValid = CheckRecording( true );
    if ( not StateValid )
    {
        return StateValid;
    }
    auto Result = Stream.Write( static_cast<UInt32>( ERHINullCommandOpcode::SetPipeline ), InPipeline->GetType() );
    if ( Result )
    {
        BoundPipeline.Emplace( InPipeline->GetType() );
        Stream.Retain( InPipeline );
    }
    return Result;
}

Lumen::TExpected<void, Lumen::FRHIError>
Lumen::Tests::FNullCommandList::SetBindingSet ( const UInt32 InIndex, TSharedRef<IRHIBindingSet> InSet ) noexcept
{
    auto StateValid = CheckRecording( true );
    if ( not StateValid )
    {
        return StateValid;
    }
    auto Result = Stream.Write( static_cast<UInt32>( ERHINullCommandOpcode::SetBindingSet ), InIndex );
    if ( Result )
    {
        Stream.Retain( InSet );
    }
    return Result;
}
Lumen::TExpected<void, Lumen::FRHIError> Lumen::Tests::FNullCommandList::SetVertexBuffer ( const UInt32 InSlot,
                                                                                           TSharedRef<IRHIBuffer> InBuffer,
                                                                                           const UInt64 InOffset ) noexcept
{
    auto StateValid = CheckRendering();
    if ( not StateValid )
    {
        return StateValid;
    }
    if ( InOffset > InBuffer->GetDesc().Size or not HasUsage( InBuffer->GetDesc().Usage, ERHIResourceUsage::VertexBuffer ) )
    {
        return TUnexpected<FRHIError>( MakeRHIInvalidArgument( ERHIErrorCondition::Command, "Invalid vertex buffer" ) );
    }
    struct FRecord
    {
        UInt32 Slot;
        UInt64 Offset;
    } Record{ .Slot = InSlot, .Offset = InOffset };
    auto Result = Stream.Write( static_cast<UInt32>( ERHINullCommandOpcode::SetVertexBuffer ), Record );
    if ( Result )
    {
        Stream.Retain( InBuffer );
    }
    return Result;
}

Lumen::TExpected<void, Lumen::FRHIError> Lumen::Tests::FNullCommandList::SetIndexBuffer (
    TSharedRef<IRHIBuffer> InBuffer, const UInt64 InOffset, const ERHIIndexFormat InFormat ) noexcept
{
    auto StateValid = CheckRendering();
    if ( not StateValid )
    {
        return StateValid;
    }
    if ( InFormat > ERHIIndexFormat::Index32 or InOffset > InBuffer->GetDesc().Size or
         not HasUsage( InBuffer->GetDesc().Usage, ERHIResourceUsage::IndexBuffer ) )
    {
        return TUnexpected<FRHIError>( MakeRHIInvalidArgument( ERHIErrorCondition::Command, "Invalid index buffer" ) );
    }
    struct FRecord
    {
        UInt64 Offset;
        ERHIIndexFormat Format;
    } Record{ .Offset = InOffset, .Format = InFormat };
    auto Result = Stream.Write( static_cast<UInt32>( ERHINullCommandOpcode::SetIndexBuffer ), Record );
    if ( Result )
    {
        Stream.Retain( InBuffer );
    }
    return Result;
}

Lumen::TExpected<void, Lumen::FRHIError> Lumen::Tests::FNullCommandList::Draw ( const FRHIDrawArguments &InArguments ) noexcept
{
    auto StateValid = CheckRendering();
    auto Valid      = InArguments.Validate();
    if ( not StateValid )
    {
        return StateValid;
    }
    if ( not Valid or not BoundPipeline or *BoundPipeline != ERHIPipelineType::Graphics )
    {
        return StateError( "Draw requires valid arguments and a graphics pipeline" );
    }
    return Stream.Write( static_cast<UInt32>( ERHINullCommandOpcode::Draw ), InArguments );
}

Lumen::TExpected<void, Lumen::FRHIError>
Lumen::Tests::FNullCommandList::DrawIndexed ( const FRHIDrawIndexedArguments &InArguments ) noexcept
{
    auto StateValid = CheckRendering();
    auto Valid      = InArguments.Validate();
    if ( not StateValid )
    {
        return StateValid;
    }
    if ( not Valid or not BoundPipeline or *BoundPipeline != ERHIPipelineType::Graphics )
    {
        return StateError( "Indexed draw requires valid arguments and a graphics pipeline" );
    }
    return Stream.Write( static_cast<UInt32>( ERHINullCommandOpcode::DrawIndexed ), InArguments );
}

Lumen::TExpected<void, Lumen::FRHIError>
Lumen::Tests::FNullCommandList::DrawIndirect ( TSharedRef<IRHIBuffer> InArguments, const FRHIIndirectArguments &InDesc ) noexcept
{
    auto StateValid = CheckRendering();
    auto Valid      = InDesc.Validate( InArguments->GetDesc().Size, sizeof( FRHIDrawArguments ) );
    if ( not StateValid )
    {
        return StateValid;
    }
    if ( not Valid or not BoundPipeline or *BoundPipeline != ERHIPipelineType::Graphics or
         not HasUsage( InArguments->GetDesc().Usage, ERHIResourceUsage::IndirectArgument ) )
    {
        return StateError( "Indirect draw requires valid arguments and a graphics pipeline" );
    }
    auto Result = Stream.Write( static_cast<UInt32>( ERHINullCommandOpcode::DrawIndirect ), InDesc );
    if ( Result )
    {
        Stream.Retain( InArguments );
    }
    return Result;
}

Lumen::TExpected<void, Lumen::FRHIError>
Lumen::Tests::FNullCommandList::Dispatch ( const FRHIDispatchArguments &InArguments ) noexcept
{
    auto StateValid = CheckRecording( false );
    auto Valid      = InArguments.Validate();
    if ( not StateValid )
    {
        return StateValid;
    }
    if ( not Valid or not BoundPipeline or *BoundPipeline != ERHIPipelineType::Compute )
    {
        return StateError( "Dispatch requires valid arguments and a compute pipeline" );
    }
    return Stream.Write( static_cast<UInt32>( ERHINullCommandOpcode::Dispatch ), InArguments );
}

Lumen::TExpected<void, Lumen::FRHIError>
Lumen::Tests::FNullCommandList::DispatchIndirect ( TSharedRef<IRHIBuffer> InArguments,
                                                   const FRHIIndirectArguments &InDesc ) noexcept
{
    auto StateValid = CheckRecording( false );
    auto Valid      = InDesc.Validate( InArguments->GetDesc().Size, sizeof( FRHIDispatchArguments ) );
    if ( not StateValid )
    {
        return StateValid;
    }
    if ( not Valid or not BoundPipeline or *BoundPipeline != ERHIPipelineType::Compute or
         not HasUsage( InArguments->GetDesc().Usage, ERHIResourceUsage::IndirectArgument ) )
    {
        return StateError( "Indirect dispatch requires valid arguments and a compute pipeline" );
    }
    auto Result = Stream.Write( static_cast<UInt32>( ERHINullCommandOpcode::DispatchIndirect ), InDesc );
    if ( Result )
    {
        Stream.Retain( InArguments );
    }
    return Result;
}

Lumen::TExpected<void, Lumen::FRHIError> Lumen::Tests::FNullCommandList::SetViewport ( const FRHIViewport &InViewport ) noexcept
{
    auto StateValid = CheckRendering();
    if ( not StateValid )
    {
        return StateValid;
    }
    if ( not InViewport.IsValid() )
    {
        return TUnexpected<FRHIError>( MakeRHIInvalidArgument( ERHIErrorCondition::Command, "Invalid viewport" ) );
    }
    return Stream.Write( static_cast<UInt32>( ERHINullCommandOpcode::SetViewport ), InViewport );
}

Lumen::TExpected<void, Lumen::FRHIError> Lumen::Tests::FNullCommandList::SetScissor ( const FRHIRect2D &InScissor ) noexcept
{
    auto StateValid = CheckRendering();
    if ( not StateValid )
    {
        return StateValid;
    }
    if ( not InScissor.IsValid() )
    {
        return TUnexpected<FRHIError>( MakeRHIInvalidArgument( ERHIErrorCondition::Command, "Invalid scissor" ) );
    }
    return Stream.Write( static_cast<UInt32>( ERHINullCommandOpcode::SetScissor ), InScissor );
}

Lumen::TExpected<void, Lumen::FRHIError>
Lumen::Tests::FNullCommandList::SetBlendConstants ( const Float32 ( &InConstants )[4] ) noexcept
{
    auto StateValid = CheckRendering();
    if ( not StateValid )
    {
        return StateValid;
    }
    TArray<Float32, 4U> Constants;
    for ( USize Index = 0U; Index < Constants.size(); ++Index )
    {
        Constants[Index] = InConstants[Index];
        if ( not std::isfinite( Constants[Index] ) )
        {
            return TUnexpected<FRHIError>( MakeRHIInvalidArgument( ERHIErrorCondition::Command, "Invalid blend constant" ) );
        }
    }
    return Stream.Write( static_cast<UInt32>( ERHINullCommandOpcode::SetBlendConstants ), Constants );
}

Lumen::TExpected<void, Lumen::FRHIError> Lumen::Tests::FNullCommandList::SetDepthBias ( const Float32 InConstant,
                                                                                        const Float32 InSlope ) noexcept
{
    auto StateValid = CheckRendering();
    if ( not StateValid )
    {
        return StateValid;
    }
    if ( not std::isfinite( InConstant ) or not std::isfinite( InSlope ) )
    {
        return TUnexpected<FRHIError>( MakeRHIInvalidArgument( ERHIErrorCondition::Command, "Invalid depth bias" ) );
    }
    const TArray<Float32, 2U> Values = { InConstant, InSlope };
    return Stream.Write( static_cast<UInt32>( ERHINullCommandOpcode::SetDepthBias ), Values );
}

Lumen::TExpected<void, Lumen::FRHIError> Lumen::Tests::FNullCommandList::SetStencilReference ( const UInt32 InReference ) noexcept
{
    auto StateValid = CheckRendering();
    if ( not StateValid )
    {
        return StateValid;
    }
    return Stream.Write( static_cast<UInt32>( ERHINullCommandOpcode::SetStencilReference ), InReference );
}
Lumen::TExpected<void, Lumen::FRHIError> Lumen::Tests::FNullCommandList::BeginQuery ( IRHIQueryPool &InPool,
                                                                                      const UInt32 InIndex ) noexcept
{
    static_cast<void>( InPool );
    auto StateValid = CheckRecording( true );
    if ( not StateValid )
    {
        return StateValid;
    }
    return Stream.Write( static_cast<UInt32>( ERHINullCommandOpcode::BeginQuery ), InIndex );
}

Lumen::TExpected<void, Lumen::FRHIError> Lumen::Tests::FNullCommandList::EndQuery ( IRHIQueryPool &InPool,
                                                                                    const UInt32 InIndex ) noexcept
{
    static_cast<void>( InPool );
    auto StateValid = CheckRecording( true );
    if ( not StateValid )
    {
        return StateValid;
    }
    return Stream.Write( static_cast<UInt32>( ERHINullCommandOpcode::EndQuery ), InIndex );
}

Lumen::TExpected<void, Lumen::FRHIError> Lumen::Tests::FNullCommandList::BeginDebugRegion ( const FStringView InName ) noexcept
{
    auto StateValid = CheckRecording( true );
    if ( not StateValid )
    {
        return StateValid;
    }
    if ( InName.empty() )
    {
        return TUnexpected<FRHIError>( MakeRHIInvalidArgument( ERHIErrorCondition::Command, "Debug region name is empty" ) );
    }
    const auto *Bytes = reinterpret_cast<const Byte *>( InName.data() );
    auto Result       = Stream.WritePayload( static_cast<UInt32>( ERHINullCommandOpcode::BeginDebugRegion ),
                                             TSpan<const Byte>( Bytes, InName.size() ) );
    if ( Result )
    {
        ++DebugDepth;
    }
    return Result;
}

Lumen::TExpected<void, Lumen::FRHIError> Lumen::Tests::FNullCommandList::EndDebugRegion () noexcept
{
    auto StateValid = CheckRecording( true );
    if ( not StateValid )
    {
        return StateValid;
    }
    if ( DebugDepth == 0U )
    {
        return StateError( "No debug region is active" );
    }
    auto Result = Stream.WritePayload( static_cast<UInt32>( ERHINullCommandOpcode::EndDebugRegion ), {} );
    if ( Result )
    {
        --DebugDepth;
    }
    return Result;
}

} // namespace Lumen
