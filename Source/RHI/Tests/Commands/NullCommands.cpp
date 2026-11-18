/**
 * @file NullCommands.cpp
 * @brief Implements deterministic Null command recording.
 */

#include "Commands/NullCommands.hpp"
#include <Queries/NullQueries.hpp>
#include <RHI/Device/RHIBackendCast.hpp>
#include <Templates/Hash.hpp>
#include <cmath>
#include <variant>

namespace Lumen
{

namespace RHI
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
            const FRHITextureDesc &Desc = InTexture.GetDesc();
            if ( InMip >= Desc.MipCount or InMip >= Desc.Extent.GetMaxMipLevelCount() or InLayer >= Desc.LayerCount or
                 not InOffset.IsNonNegative() )
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

    Lumen::RHI::Tests::FNullCommandAllocator::FNullCommandAllocator ( const ERHIQueueType InQueueType,
                                                                      const FRHIObjectId InId ) noexcept
        : FRHICommandAllocatorBase( InQueueType ), Identity( InId, FHash::TypeHash<FNullCommandAllocator>() )
    {
        /* */
    }

    LUMEN_RHI_OBJECT_FORWARDERS( Tests::FNullCommandAllocator )
    Lumen::RHI::Tests::FNullCommandList::FNullCommandList ( TSharedPtr<FNullCommandAllocator> InAllocator,
                                                            const FRHIObjectId InId ) noexcept
        : Identity( InId, FHash::TypeHash<FNullCommandList>() ), Allocator( std::move( InAllocator ) )
    {
        Allocator->RegisterList( *this );
    }

    Lumen::RHI::Tests::FNullCommandList::~FNullCommandList () noexcept
    {
        Allocator->UnregisterList( *this );
    }

    LUMEN_RHI_OBJECT_FORWARDERS( Tests::FNullCommandList )
    Lumen::RHI::ERHIQueueType Lumen::RHI::Tests::FNullCommandList::GetQueueType () const noexcept
    {
        return Allocator->GetQueueType();
    }

    Lumen::RHI::ERHICommandListState Lumen::RHI::Tests::FNullCommandList::GetState () const noexcept
    {
        return State;
    }

    Lumen::RHI::FRHITimelineValue Lumen::RHI::Tests::FNullCommandList::GetCompletion () const noexcept
    {
        return Completion;
    }

    const Lumen::RHI::FRHICommandStream &Lumen::RHI::Tests::FNullCommandList::GetStream () const noexcept
    {
        return Stream;
    }

    Lumen::TExpected<void, Lumen::RHI::FRHIError> Lumen::RHI::Tests::FNullCommandList::Begin () noexcept
    {
        return RHITransitionCommandList( State, ERHICommandListState::Recording );
    }

    Lumen::TExpected<void, Lumen::RHI::FRHIError> Lumen::RHI::Tests::FNullCommandList::End () noexcept
    {
        if ( ActiveQueryPool != nullptr )
        {
            return StateError( "Cannot end a command list with an active query" );
        }
        return RHITransitionCommandList( State, ERHICommandListState::Executable );
    }

    Lumen::TExpected<void, Lumen::RHI::FRHIError> Lumen::RHI::Tests::FNullCommandList::Reset () noexcept
    {
        if ( State == ERHICommandListState::Pending )
        {
            return StateError( "Cannot reset a pending Null command list" );
        }
        if ( ActiveQueryPool != nullptr )
        {
            return StateError( "Cannot reset a command list with an active query" );
        }
        if ( State != ERHICommandListState::Initial )
        {
            return RHITransitionCommandList( State, ERHICommandListState::Initial )
                .AndThen( [this] () noexcept -> TExpected<void, FRHIError>
            {
                Stream.Clear();
                Completion = {};
                BoundPipeline.Reset();
                DebugDepth = 0U;
                return {};
            } );
        }
        Stream.Clear();
        Completion = {};
        BoundPipeline.Reset();
        DebugDepth = 0U;
        return {};
    }

    Lumen::TExpected<void, Lumen::RHI::FRHIError>
    Lumen::RHI::Tests::FNullCommandList::MarkPending ( const FRHITimelineValue InCompletion ) noexcept
    {
        if ( not InCompletion.IsValid() )
        {
            return TUnexpected<FRHIError>( MakeRHIInvalidArgument( ERHIErrorCondition::Command, "Pending completion is zero" ) );
        }
        return RHITransitionCommandList( State, ERHICommandListState::Pending )
            .Inspect( [this, InCompletion] () noexcept
        {
            Completion = InCompletion;
        } );
    }

    Lumen::TExpected<void, Lumen::RHI::FRHIError>
    Lumen::RHI::Tests::FNullCommandList::MarkCompleted ( const FRHITimelineValue InCompleted ) noexcept
    {
        if ( State == ERHICommandListState::Pending and not IsRHICompletionReached( InCompleted, Completion ) )
        {
            return StateError( "Command list completion has not been reached" );
        }
        return RHITransitionCommandList( State, ERHICommandListState::Executable )
            .Inspect( [this] () noexcept
        {
            Completion = {};
        } );
    }

    Lumen::TExpected<void, Lumen::RHI::FRHIError>
    Lumen::RHI::Tests::FNullCommandList::CheckRecording ( const Bool bAllowRendering ) const noexcept
    {
        if ( State != ERHICommandListState::Recording and ( not bAllowRendering or State != ERHICommandListState::Rendering ) )
        {
            return StateError( "Command is outside a recording scope" );
        }
        return {};
    }

    Lumen::TExpected<void, Lumen::RHI::FRHIError> Lumen::RHI::Tests::FNullCommandList::CheckRendering () const noexcept
    {
        if ( State != ERHICommandListState::Rendering )
        {
            return StateError( "Command requires an active rendering scope" );
        }
        return {};
    }

    Lumen::TExpected<void, Lumen::RHI::FRHIError>
    Lumen::RHI::Tests::FNullCommandList::Transition ( const TSpan<const FRHITransition> InTransitions ) noexcept
    {
        const TExpected<void, FRHIError> StateValid = CheckRecording( true );
        if ( not StateValid )
        {
            return StateValid;
        }
        for ( const auto &Transition : InTransitions )
        {
            const TExpected<void, FRHIError> Valid = ValidateRHITransition( Transition );
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
            if ( const FRHIBufferTransition *Buffer = std::get_if<FRHIBufferTransition>( &Transition ) )
            {
                Record = { .Before = Buffer->Before, .After = Buffer->After };
                Stream.Retain( StaticCastSharedPtr<IRHIObject>( Buffer->Buffer ) );
            }
            else if ( const FRHITextureTransition *Texture = std::get_if<FRHITextureTransition>( &Transition ) )
            {
                Record = { .Before = Texture->Before, .After = Texture->After };
                Stream.Retain( StaticCastSharedPtr<IRHIObject>( Texture->Texture ) );
            }
            else
            {
                return TUnexpected<FRHIError>(
                    MakeRHICorrupted( ERHIErrorCondition::Command, "Transition variant has no active payload" ) );
            }
            const TExpected<void, FRHIError> Result =
                Stream.Write( static_cast<UInt32>( ERHINullCommandOpcode::Transition ), Record );
            if ( not Result )
            {
                return Result;
            }
        }
        return {};
    }

    Lumen::TExpected<void, Lumen::RHI::FRHIError> Lumen::RHI::Tests::FNullCommandList::CopyBuffer (
        TSharedRef<IRHIBuffer> InSource, TSharedRef<IRHIBuffer> InDestination, const FRHIBufferCopy &InCopy ) noexcept
    {
        const TExpected<void, FRHIError> StateValid = CheckRecording( true );
        const TExpected<void, FRHIError> Valid =
            InCopy.Validate( InSource->GetDesc().Size, InDestination->GetDesc().Size, InSource.Get() == InDestination.Get() );
        if ( not StateValid )
        {
            return StateValid;
        }
        if ( not Valid or not HasUsage( InSource->GetDesc().Usage, ERHIResourceUsage::CopySource ) or
             not HasUsage( InDestination->GetDesc().Usage, ERHIResourceUsage::CopyDestination ) )
        {
            return TUnexpected<FRHIError>(
                MakeRHIInvalidArgument( ERHIErrorCondition::Command, "Invalid buffer copy or usage" ) );
        }
        return Stream.Write( static_cast<UInt32>( ERHINullCommandOpcode::CopyBuffer ), InCopy )
            .Inspect( [this, &InSource, &InDestination] () noexcept
        {
            Stream.Retain( InSource );
            Stream.Retain( InDestination );
        } );
    }

    Lumen::TExpected<void, Lumen::RHI::FRHIError> Lumen::RHI::Tests::FNullCommandList::CopyTexture (
        TSharedRef<IRHITexture> InSource, TSharedRef<IRHITexture> InDestination, const FRHITextureCopy &InCopy ) noexcept
    {
        const TExpected<void, FRHIError> StateValid = CheckRecording( true );
        const TExpected<void, FRHIError> Valid      = InCopy.Validate( InSource->GetDesc().Format );
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
            return TUnexpected<FRHIError>(
                MakeRHIInvalidArgument( ERHIErrorCondition::Command, "Invalid texture copy or usage" ) );
        }
        return Stream.Write( static_cast<UInt32>( ERHINullCommandOpcode::CopyTexture ), InCopy )
            .Inspect( [this, &InSource, &InDestination] () noexcept
        {
            Stream.Retain( InSource );
            Stream.Retain( InDestination );
        } );
    }

    Lumen::TExpected<void, Lumen::RHI::FRHIError> Lumen::RHI::Tests::FNullCommandList::CopyBufferToTexture (
        TSharedRef<IRHIBuffer> InSource, TSharedRef<IRHITexture> InDestination, const FRHIBufferTextureCopy &InCopy ) noexcept
    {
        const TExpected<void, FRHIError> StateValid = CheckRecording( true );
        const TExpected<void, FRHIError> Valid      = InCopy.Validate( InDestination->GetDesc().Format );
        if ( not StateValid )
        {
            return StateValid;
        }
        if ( not Valid or
             not IsTextureRegionValid( *InDestination, InCopy.Mip, InCopy.Layer, InCopy.TextureOffset, InCopy.Extent ) or
             not HasUsage( InSource->GetDesc().Usage, ERHIResourceUsage::CopySource ) or
             not HasUsage( InDestination->GetDesc().Usage, ERHIResourceUsage::CopyDestination ) )
        {
            return TUnexpected<FRHIError>(
                MakeRHIInvalidArgument( ERHIErrorCondition::Command, "Invalid buffer-to-texture copy" ) );
        }
        return Stream.Write( static_cast<UInt32>( ERHINullCommandOpcode::CopyBufferToTexture ), InCopy )
            .Inspect( [this, &InSource, &InDestination] () noexcept
        {
            Stream.Retain( InSource );
            Stream.Retain( InDestination );
        } );
    }

    Lumen::TExpected<void, Lumen::RHI::FRHIError> Lumen::RHI::Tests::FNullCommandList::CopyTextureToBuffer (
        TSharedRef<IRHITexture> InSource, TSharedRef<IRHIBuffer> InDestination, const FRHIBufferTextureCopy &InCopy ) noexcept
    {
        const TExpected<void, FRHIError> StateValid = CheckRecording( true );
        const TExpected<void, FRHIError> Valid      = InCopy.Validate( InSource->GetDesc().Format );
        if ( not StateValid )
        {
            return StateValid;
        }
        if ( not Valid or not IsTextureRegionValid( *InSource, InCopy.Mip, InCopy.Layer, InCopy.TextureOffset, InCopy.Extent ) or
             not HasUsage( InSource->GetDesc().Usage, ERHIResourceUsage::CopySource ) or
             not HasUsage( InDestination->GetDesc().Usage, ERHIResourceUsage::CopyDestination ) )
        {
            return TUnexpected<FRHIError>(
                MakeRHIInvalidArgument( ERHIErrorCondition::Command, "Invalid texture-to-buffer copy" ) );
        }
        return Stream.Write( static_cast<UInt32>( ERHINullCommandOpcode::CopyTextureToBuffer ), InCopy )
            .Inspect( [this, &InSource, &InDestination] () noexcept
        {
            Stream.Retain( InSource );
            Stream.Retain( InDestination );
        } );
    }

    Lumen::TExpected<void, Lumen::RHI::FRHIError>
    Lumen::RHI::Tests::FNullCommandList::BeginRendering ( const FRHIRenderingDesc &InDesc ) noexcept
    {
        if ( GetQueueType() != ERHIQueueType::Graphics )
        {
            return StateError( "Rendering requires a graphics command list" );
        }
        const TExpected<void, FRHIError> StateValid = CheckRecording( false );
        const TExpected<void, FRHIError> Valid      = InDesc.Validate();
        if ( not StateValid )
        {
            return StateValid;
        }
        if ( not Valid )
        {
            return Valid;
        }
        return Stream.Write( static_cast<UInt32>( ERHINullCommandOpcode::BeginRendering ), InDesc.Area )
            .AndThen( [this, &InDesc] () noexcept -> TExpected<void, FRHIError>
        {
            for ( const FRHIRenderingAttachment &Attachment : InDesc.ColorAttachments )
            {
                Stream.Retain( StaticCastSharedPtr<IRHIObject>( Attachment.View ) );
            }
            if ( InDesc.DepthStencilAttachment.View )
            {
                Stream.Retain( StaticCastSharedPtr<IRHIObject>( InDesc.DepthStencilAttachment.View ) );
            }
            return RHITransitionCommandList( State, ERHICommandListState::Rendering );
        } );
    }

    Lumen::TExpected<void, Lumen::RHI::FRHIError> Lumen::RHI::Tests::FNullCommandList::EndRendering () noexcept
    {
        const TExpected<void, FRHIError> StateValid = CheckRendering();
        if ( not StateValid )
        {
            return StateValid;
        }
        return Stream.WritePayload( static_cast<UInt32>( ERHINullCommandOpcode::EndRendering ), {} )
            .AndThen( [this] () noexcept -> TExpected<void, FRHIError>
        {
            return RHITransitionCommandList( State, ERHICommandListState::Recording );
        } );
    }

    Lumen::TExpected<void, Lumen::RHI::FRHIError>
    Lumen::RHI::Tests::FNullCommandList::SetPipeline ( TSharedRef<IRHIPipeline> InPipeline ) noexcept
    {
        const TExpected<void, FRHIError> StateValid = CheckRecording( true );
        if ( not StateValid )
        {
            return StateValid;
        }
        return Stream.Write( static_cast<UInt32>( ERHINullCommandOpcode::SetPipeline ), InPipeline->GetType() )
            .Inspect( [this, &InPipeline] () noexcept
        {
            BoundPipeline.Emplace( InPipeline->GetType() );
            Stream.Retain( InPipeline );
        } );
    }

    Lumen::TExpected<void, Lumen::RHI::FRHIError>
    Lumen::RHI::Tests::FNullCommandList::SetBindingSet ( const UInt32 InIndex, TSharedRef<IRHIBindingSet> InSet ) noexcept
    {
        const TExpected<void, FRHIError> StateValid = CheckRecording( true );
        if ( not StateValid )
        {
            return StateValid;
        }
        return Stream.Write( static_cast<UInt32>( ERHINullCommandOpcode::SetBindingSet ), InIndex )
            .Inspect( [this, &InSet] () noexcept
        {
            Stream.Retain( InSet );
        } );
    }
    Lumen::TExpected<void, Lumen::RHI::FRHIError> Lumen::RHI::Tests::FNullCommandList::SetVertexBuffer (
        const UInt32 InSlot, TSharedRef<IRHIBuffer> InBuffer, const UInt64 InOffset ) noexcept
    {
        const TExpected<void, FRHIError> StateValid = CheckRendering();
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
        return Stream.Write( static_cast<UInt32>( ERHINullCommandOpcode::SetVertexBuffer ), Record )
            .Inspect( [this, &InBuffer] () noexcept
        {
            Stream.Retain( InBuffer );
        } );
    }

    Lumen::TExpected<void, Lumen::RHI::FRHIError> Lumen::RHI::Tests::FNullCommandList::SetIndexBuffer (
        TSharedRef<IRHIBuffer> InBuffer, const UInt64 InOffset, const ERHIIndexFormat InFormat ) noexcept
    {
        const TExpected<void, FRHIError> StateValid = CheckRendering();
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
        return Stream.Write( static_cast<UInt32>( ERHINullCommandOpcode::SetIndexBuffer ), Record )
            .Inspect( [this, &InBuffer] () noexcept
        {
            Stream.Retain( InBuffer );
        } );
    }

    Lumen::TExpected<void, Lumen::RHI::FRHIError>
    Lumen::RHI::Tests::FNullCommandList::Draw ( const FRHIDrawArguments &InArguments ) noexcept
    {
        const TExpected<void, FRHIError> StateValid = CheckRendering();
        const TExpected<void, FRHIError> Valid      = InArguments.Validate();
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

    Lumen::TExpected<void, Lumen::RHI::FRHIError>
    Lumen::RHI::Tests::FNullCommandList::DrawIndexed ( const FRHIDrawIndexedArguments &InArguments ) noexcept
    {
        const TExpected<void, FRHIError> StateValid = CheckRendering();
        const TExpected<void, FRHIError> Valid      = InArguments.Validate();
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

    Lumen::TExpected<void, Lumen::RHI::FRHIError>
    Lumen::RHI::Tests::FNullCommandList::DrawIndirect ( TSharedRef<IRHIBuffer> InArguments,
                                                        const FRHIIndirectArguments &InDesc ) noexcept
    {
        const TExpected<void, FRHIError> StateValid = CheckRendering();
        const TExpected<void, FRHIError> Valid      = InDesc.Validate( InArguments->GetDesc().Size, sizeof( FRHIDrawArguments ) );
        if ( not StateValid )
        {
            return StateValid;
        }
        if ( not Valid or not BoundPipeline or *BoundPipeline != ERHIPipelineType::Graphics or
             not HasUsage( InArguments->GetDesc().Usage, ERHIResourceUsage::IndirectArgument ) )
        {
            return StateError( "Indirect draw requires valid arguments and a graphics pipeline" );
        }
        return Stream.Write( static_cast<UInt32>( ERHINullCommandOpcode::DrawIndirect ), InDesc )
            .Inspect( [this, &InArguments] () noexcept
        {
            Stream.Retain( InArguments );
        } );
    }

    Lumen::TExpected<void, Lumen::RHI::FRHIError>
    Lumen::RHI::Tests::FNullCommandList::Dispatch ( const FRHIDispatchArguments &InArguments ) noexcept
    {
        const TExpected<void, FRHIError> StateValid = CheckRecording( false );
        const TExpected<void, FRHIError> Valid      = InArguments.Validate();
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

    Lumen::TExpected<void, Lumen::RHI::FRHIError>
    Lumen::RHI::Tests::FNullCommandList::DispatchIndirect ( TSharedRef<IRHIBuffer> InArguments,
                                                            const FRHIIndirectArguments &InDesc ) noexcept
    {
        const TExpected<void, FRHIError> StateValid = CheckRecording( false );
        const TExpected<void, FRHIError> Valid = InDesc.Validate( InArguments->GetDesc().Size, sizeof( FRHIDispatchArguments ) );
        if ( not StateValid )
        {
            return StateValid;
        }
        if ( not Valid or not BoundPipeline or *BoundPipeline != ERHIPipelineType::Compute or
             not HasUsage( InArguments->GetDesc().Usage, ERHIResourceUsage::IndirectArgument ) )
        {
            return StateError( "Indirect dispatch requires valid arguments and a compute pipeline" );
        }
        return Stream.Write( static_cast<UInt32>( ERHINullCommandOpcode::DispatchIndirect ), InDesc )
            .Inspect( [this, &InArguments] () noexcept
        {
            Stream.Retain( InArguments );
        } );
    }

    Lumen::TExpected<void, Lumen::RHI::FRHIError>
    Lumen::RHI::Tests::FNullCommandList::SetViewport ( const FRHIViewport &InViewport ) noexcept
    {
        const TExpected<void, FRHIError> StateValid = CheckRendering();
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

    Lumen::TExpected<void, Lumen::RHI::FRHIError>
    Lumen::RHI::Tests::FNullCommandList::SetScissor ( const FRHIRect2D &InScissor ) noexcept
    {
        const TExpected<void, FRHIError> StateValid = CheckRendering();
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

    Lumen::TExpected<void, Lumen::RHI::FRHIError>
    Lumen::RHI::Tests::FNullCommandList::SetBlendConstants ( const Float32 ( &InConstants )[4] ) noexcept
    {
        const TExpected<void, FRHIError> StateValid = CheckRendering();
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

    Lumen::TExpected<void, Lumen::RHI::FRHIError>
    Lumen::RHI::Tests::FNullCommandList::SetDepthBias ( const Float32 InConstant, const Float32 InSlope ) noexcept
    {
        const TExpected<void, FRHIError> StateValid = CheckRendering();
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

    Lumen::TExpected<void, Lumen::RHI::FRHIError>
    Lumen::RHI::Tests::FNullCommandList::SetStencilReference ( const UInt32 InReference ) noexcept
    {
        const TExpected<void, FRHIError> StateValid = CheckRendering();
        if ( not StateValid )
        {
            return StateValid;
        }
        return Stream.Write( static_cast<UInt32>( ERHINullCommandOpcode::SetStencilReference ), InReference );
    }
    Lumen::TExpected<void, Lumen::RHI::FRHIError>
    Lumen::RHI::Tests::FNullCommandList::BeginQuery ( TSharedRef<IRHIQueryPool> InPool, const UInt32 InIndex ) noexcept
    {
        const TExpected<void, FRHIError> StateValid = CheckRecording( true );
        if ( not StateValid )
        {
            return StateValid;
        }
        const TOptional<TSharedPtr<FNullQueryPool>> PoolRef =
            TryRHIBackendCast<FNullQueryPool>( StaticCastSharedRef<IRHIObject>( InPool ) );
        if ( not PoolRef or InPool->GetDesc().QueueType != GetQueueType() or ActiveQueryPool != nullptr )
        {
            return StateError( "Query pool is foreign, incompatible with the queue, or another query is active" );
        }
        FNullQueryPool *const Pool             = PoolRef->Get();
        const TExpected<void, FRHIError> Begun = Pool->Begin( InIndex );
        if ( not Begun )
        {
            return Begun;
        }
        return Stream.Write( static_cast<UInt32>( ERHINullCommandOpcode::BeginQuery ), InIndex )
            .Inspect( [this, &InPool, Pool, InIndex] () noexcept
        {
            Stream.Retain( StaticCastSharedRef<IRHIObject>( InPool ) );
            ActiveQueryPool  = Pool;
            ActiveQueryIndex = InIndex;
        } );
    }

    Lumen::TExpected<void, Lumen::RHI::FRHIError>
    Lumen::RHI::Tests::FNullCommandList::EndQuery ( TSharedRef<IRHIQueryPool> InPool, const UInt32 InIndex ) noexcept
    {
        const TExpected<void, FRHIError> StateValid = CheckRecording( true );
        if ( not StateValid )
        {
            return StateValid;
        }
        const TOptional<TSharedPtr<FNullQueryPool>> PoolRef =
            TryRHIBackendCast<FNullQueryPool>( StaticCastSharedRef<IRHIObject>( InPool ) );
        if ( not PoolRef or PoolRef->Get() != ActiveQueryPool or InIndex != ActiveQueryIndex )
        {
            return StateError( "Ending query does not match the active query" );
        }
        FNullQueryPool *const Pool             = PoolRef->Get();
        const TExpected<void, FRHIError> Ended = Pool->End( InIndex );
        if ( not Ended )
        {
            return Ended;
        }
        return Stream.Write( static_cast<UInt32>( ERHINullCommandOpcode::EndQuery ), InIndex )
            .Inspect( [this] () noexcept
        {
            ActiveQueryPool  = nullptr;
            ActiveQueryIndex = 0U;
        } );
    }

    Lumen::TExpected<void, Lumen::RHI::FRHIError>
    Lumen::RHI::Tests::FNullCommandList::BeginDebugRegion ( const FStringView InName ) noexcept
    {
        const TExpected<void, FRHIError> StateValid = CheckRecording( true );
        if ( not StateValid )
        {
            return StateValid;
        }
        if ( InName.empty() )
        {
            return TUnexpected<FRHIError>( MakeRHIInvalidArgument( ERHIErrorCondition::Command, "Debug region name is empty" ) );
        }
        const Byte *Bytes = reinterpret_cast<const Byte *>( InName.data() );
        return Stream
            .WritePayload( static_cast<UInt32>( ERHINullCommandOpcode::BeginDebugRegion ),
                           TSpan<const Byte>( Bytes, InName.size() ) )
            .Inspect( [this] () noexcept
        {
            ++DebugDepth;
        } );
    }

    Lumen::TExpected<void, Lumen::RHI::FRHIError> Lumen::RHI::Tests::FNullCommandList::EndDebugRegion () noexcept
    {
        const TExpected<void, FRHIError> StateValid = CheckRecording( true );
        if ( not StateValid )
        {
            return StateValid;
        }
        if ( DebugDepth == 0U )
        {
            return StateError( "No debug region is active" );
        }
        return Stream.WritePayload( static_cast<UInt32>( ERHINullCommandOpcode::EndDebugRegion ), {} )
            .Inspect( [this] () noexcept
        {
            --DebugDepth;
        } );
    }

} // namespace RHI

} // namespace Lumen
