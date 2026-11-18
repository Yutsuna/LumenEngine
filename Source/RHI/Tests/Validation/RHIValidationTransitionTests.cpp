/**
 * @file RHIValidationTransitionTests.cpp
 * @brief Tests tracked buffer, subresource, and queue-transfer transitions.
 */
#include "Validation/RHIValidationDevice.hpp"
#include "Validation/RHIValidationTestUtils.hpp"
#include <Tests/UnitTest.hpp>

namespace Lumen
{

namespace RHI
{
    namespace
    {
        LUMEN_TEST( RHIValidationTransition, RejectsMismatchedBufferStateWithoutRecording )
        {
            Tests::FRecordingDebugSink Sink;
            TSharedRef<IRHIDevice> Device = Tests::Validation::MakeDevice( Sink );
            TSharedRef<IRHIBuffer> Buffer = Tests::Validation::MakeBuffer( Device, 64ULL, ERHIResourceUsage::CopySource );
            TSharedRef<IRHICommandAllocator> Allocator = *Device->CreateCommandAllocator( ERHIQueueType::Graphics );
            TSharedRef<IRHICommandList> List           = *Device->CreateCommandList( Allocator );
            LUMEN_CHECK_TRUE( List->Begin().HasValue() );
            FRHIBufferTransition BufferTransition;
            BufferTransition.Buffer   = Buffer;
            BufferTransition.Before   = ERHIResourceState::CopySource;
            BufferTransition.After    = ERHIResourceState::CopyDestination;
            FRHITransition Transition = BufferTransition;
            LUMEN_CHECK_EQ( List->Transition( TSpan<const FRHITransition>( &Transition, 1U ) ).Error().Code,
                            ERHIErrorCode::ValidationFailed );
            LUMEN_CHECK_EQ( List->GetStream().GetCommandCount(), 0U );
            BufferTransition.Before = ERHIResourceState::Common;
            BufferTransition.After  = ERHIResourceState::CopySource;
            Transition              = BufferTransition;
            LUMEN_CHECK_TRUE( List->Transition( TSpan<const FRHITransition>( &Transition, 1U ) ).HasValue() );
            auto *Validation = dynamic_cast<FRHIValidationDevice *>( Device.Get() );
            LUMEN_CHECK_EQ( *Validation->GetState()->GetResourceState( *Buffer ), ERHIResourceState::CopySource );
            Transition = BufferTransition;
            LUMEN_CHECK_EQ( List->Transition( TSpan<const FRHITransition>( &Transition, 1U ) ).Error().Code,
                            ERHIErrorCode::ValidationFailed );
            FRHIBufferTransition ValidTransition   = BufferTransition;
            ValidTransition.Before                 = ERHIResourceState::CopySource;
            ValidTransition.After                  = ERHIResourceState::CopyDestination;
            FRHIBufferTransition InvalidTransition = BufferTransition;
            TArray<FRHITransition, 2U> Batch       = { FRHITransition( ValidTransition ), FRHITransition( InvalidTransition ) };
            LUMEN_CHECK_EQ( List->Transition( Batch ).Error().Code, ERHIErrorCode::ValidationFailed );
            LUMEN_CHECK_EQ( *Validation->GetState()->GetResourceState( *Buffer ), ERHIResourceState::CopySource );
            LUMEN_CHECK_EQ( List->GetStream().GetCommandCount(), 1U );
        }

        LUMEN_TEST( RHIValidationTransition, RejectsSequencedBatchBeforeBackendRecordsIt )
        {
            Tests::FRecordingDebugSink Sink;
            TSharedRef<IRHIDevice> Device              = Tests::Validation::MakeDevice( Sink );
            TSharedRef<IRHIBuffer> Buffer              = Tests::Validation::MakeBuffer( Device );
            TSharedRef<IRHICommandAllocator> Allocator = *Device->CreateCommandAllocator( ERHIQueueType::Graphics );
            TSharedRef<IRHICommandList> List           = *Device->CreateCommandList( Allocator );
            LUMEN_CHECK_TRUE( List->Begin().HasValue() );

            FRHIBufferTransition First;
            First.Buffer = Buffer;
            First.Before = ERHIResourceState::Common;
            First.After  = ERHIResourceState::CopySource;
            FRHIBufferTransition Second;
            Second.Buffer                    = Buffer;
            Second.Before                    = ERHIResourceState::Common;
            Second.After                     = ERHIResourceState::CopyDestination;
            TArray<FRHITransition, 2U> Batch = { FRHITransition( First ), FRHITransition( Second ) };

            const USize RecordedBefore = List->GetStream().GetCommandCount();
            LUMEN_CHECK_EQ( List->Transition( Batch ).Error().Code, ERHIErrorCode::ValidationFailed );
            LUMEN_CHECK_EQ( List->GetStream().GetCommandCount(), RecordedBefore );
            auto *Validation = dynamic_cast<FRHIValidationDevice *>( Device.Get() );
            LUMEN_CHECK_EQ( *Validation->GetState()->GetResourceState( *Buffer ), ERHIResourceState::Common );

            Second.Before = ERHIResourceState::CopySource;
            Batch         = { FRHITransition( First ), FRHITransition( Second ) };
            LUMEN_CHECK_TRUE( List->Transition( Batch ).HasValue() );
            LUMEN_CHECK_EQ( List->GetStream().GetCommandCount(), RecordedBefore + 2U );
            LUMEN_CHECK_EQ( *Validation->GetState()->GetResourceState( *Buffer ), ERHIResourceState::CopyDestination );
        }

        LUMEN_TEST( RHIValidationTransition, ClearedResourceStatesFlagSuppressesStateRejection )
        {
            Tests::FRecordingDebugSink Sink;
            FRHIValidationDesc Desc;
            Desc.Flags                                 = ERHIValidationFlags::All & ~ERHIValidationFlags::ResourceStates;
            TSharedRef<IRHIDevice> Device              = Tests::Validation::MakeDevice( Sink, Desc );
            TSharedRef<IRHIBuffer> Buffer              = Tests::Validation::MakeBuffer( Device );
            TSharedRef<IRHICommandAllocator> Allocator = *Device->CreateCommandAllocator( ERHIQueueType::Graphics );
            TSharedRef<IRHICommandList> List           = *Device->CreateCommandList( Allocator );
            LUMEN_CHECK_TRUE( List->Begin().HasValue() );

            FRHIBufferTransition BufferTransition;
            BufferTransition.Buffer   = Buffer;
            BufferTransition.Before   = ERHIResourceState::CopySource;
            BufferTransition.After    = ERHIResourceState::CopyDestination;
            FRHITransition Transition = BufferTransition;
            LUMEN_CHECK_TRUE( List->Transition( TSpan<const FRHITransition>( &Transition, 1U ) ).HasValue() );
            LUMEN_CHECK_EQ( List->GetStream().GetCommandCount(), 1U );
        }

        LUMEN_TEST( RHIValidationTransition, TracksTextureMipsAndLayersIndependently )
        {
            Tests::FRecordingDebugSink Sink;
            TSharedRef<IRHIDevice> Device              = Tests::Validation::MakeDevice( Sink );
            TSharedRef<IRHITexture> Texture            = Tests::Validation::MakeTexture( Device, 2U, 2U );
            TSharedRef<IRHICommandAllocator> Allocator = *Device->CreateCommandAllocator( ERHIQueueType::Graphics );
            TSharedRef<IRHICommandList> List           = *Device->CreateCommandList( Allocator );
            LUMEN_CHECK_TRUE( List->Begin().HasValue() );
            FRHITextureTransition TextureTransition;
            TextureTransition.Texture = Texture;
            TextureTransition.Range   = { .BaseMip = 1U, .MipCount = 1U, .BaseLayer = 1U, .LayerCount = 1U };
            TextureTransition.Before  = ERHIResourceState::Common;
            TextureTransition.After   = ERHIResourceState::CopySource;
            FRHITransition Transition = TextureTransition;
            LUMEN_CHECK_TRUE( List->Transition( TSpan<const FRHITransition>( &Transition, 1U ) ).HasValue() );
            auto *Validation  = dynamic_cast<FRHIValidationDevice *>( Device.Get() );
            const auto &State = Validation->GetState();
            LUMEN_CHECK_EQ( *State->GetTextureState( *Texture, 1U, 1U ), ERHIResourceState::CopySource );
            LUMEN_CHECK_EQ( *State->GetTextureState( *Texture, 0U, 0U ), ERHIResourceState::Common );
            LUMEN_CHECK_FALSE( State->GetResourceState( *Texture ).HasValue() );
            TextureTransition.Range.BaseMip = 2U;
            Transition                      = TextureTransition;
            LUMEN_CHECK_EQ( List->Transition( TSpan<const FRHITransition>( &Transition, 1U ) ).Error().Code,
                            ERHIErrorCode::ValidationFailed );
        }

        LUMEN_TEST( RHIValidationTransition, ValidatesQueueOwnershipTransfers )
        {
            Tests::FRecordingDebugSink Sink;
            TSharedRef<IRHIDevice> Device              = Tests::Validation::MakeDevice( Sink );
            TSharedRef<IRHIBuffer> Buffer              = Tests::Validation::MakeBuffer( Device );
            TSharedRef<IRHICommandAllocator> Allocator = *Device->CreateCommandAllocator( ERHIQueueType::Graphics );
            TSharedRef<IRHICommandList> List           = *Device->CreateCommandList( Allocator );
            LUMEN_CHECK_TRUE( List->Begin().HasValue() );
            FRHIBufferTransition BufferTransition;
            BufferTransition.Buffer           = Buffer;
            BufferTransition.Before           = ERHIResourceState::Common;
            BufferTransition.After            = ERHIResourceState::CopySource;
            BufferTransition.SourceQueue      = ERHIQueueType::Graphics;
            BufferTransition.DestinationQueue = ERHIQueueType::Compute;
            FRHITransition Transition         = BufferTransition;
            LUMEN_CHECK_TRUE( List->Transition( TSpan<const FRHITransition>( &Transition, 1U ) ).HasValue() );
            BufferTransition.Before = ERHIResourceState::CopySource;
            BufferTransition.After  = ERHIResourceState::CopyDestination;
            Transition              = BufferTransition;
            LUMEN_CHECK_EQ( List->Transition( TSpan<const FRHITransition>( &Transition, 1U ) ).Error().Code,
                            ERHIErrorCode::ValidationFailed );
            BufferTransition.SourceQueue      = ERHIQueueType::Compute;
            BufferTransition.DestinationQueue = ERHIQueueType::Compute;
            Transition                        = BufferTransition;
            LUMEN_CHECK_EQ( List->Transition( TSpan<const FRHITransition>( &Transition, 1U ) ).Error().Code,
                            ERHIErrorCode::ValidationFailed );
        }
    } // namespace
} // namespace RHI

} // namespace Lumen
