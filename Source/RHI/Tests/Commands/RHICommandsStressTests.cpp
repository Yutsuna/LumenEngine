/**
 * @file RHICommandsStressTests.cpp
 * @brief Stresses long command streams and repeated recording/recycling.
 */

#include <Device/NullDevice.hpp>
#include <RHI/Commands/IRHICommandAllocator.hpp>
#include <RHI/Commands/IRHICommandList.hpp>
#include <RHI/Commands/RHICommandStream.hpp>
#include <RHI/Queue/IRHIQueue.hpp>
#include <Tests/UnitTest.hpp>

namespace Lumen
{

namespace RHI
{
    namespace
    {

        LUMEN_TEST( RHICommandsStress, LongChunkedStreamsPreserveEveryPayload )
        {
            FRHICommandStream Stream( 64U );
            for ( UInt32 Index = 0U; Index < 1024U; ++Index )
            {
                const FRHIDispatchArguments Arguments{ .GroupCountX = Index + 1U, .GroupCountY = 2U, .GroupCountZ = 3U };
                LUMEN_CHECK_TRUE( Stream.Write( Index % 17U, Arguments ).HasValue() );
            }
            LUMEN_CHECK_EQ( Stream.GetCommandCount(), 1024U );
            LUMEN_CHECK_TRUE( Stream.GetChunkCount() > 16U );
            for ( USize Index = 0U; Index < Stream.GetCommandCount(); ++Index )
            {
                const UInt32 Opcode = static_cast<UInt32>( Index ) % 17U;
                const TExpected<FRHIDispatchArguments, FRHIError> Arguments =
                    Stream.ReadPayload<FRHIDispatchArguments>( Index, Opcode );
                LUMEN_CHECK_TRUE( Arguments.HasValue() );
                LUMEN_CHECK_EQ( Arguments->GroupCountX, static_cast<UInt32>( Index ) + 1U );
                LUMEN_CHECK_EQ( Arguments->GroupCountY, 2U );
                LUMEN_CHECK_EQ( Arguments->GroupCountZ, 3U );
            }
            const USize AllocatedChunks = Stream.GetChunkCount();
            Stream.Clear();
            LUMEN_CHECK_EQ( Stream.GetCommandCount(), 0U );
            LUMEN_CHECK_EQ( Stream.GetChunkCount(), AllocatedChunks );
            for ( UInt32 Index = 0U; Index < 256U; ++Index )
            {
                LUMEN_CHECK_TRUE( Stream.Write( 1U, Index ).HasValue() );
            }
            LUMEN_CHECK_EQ( Stream.GetCommandCount(), 256U );
        }

        LUMEN_TEST( RHICommandsStress, RepeatedRecordSubmitResetKeepsStreamsBounded )
        {
            Tests::FNullDevice Device;
            TSharedRef<IRHICommandAllocator> Allocator = *Device.CreateCommandAllocator( ERHIQueueType::Graphics );
            TSharedRef<IRHICommandList> List           = *Device.CreateCommandList( Allocator );
            TSharedRef<IRHIBuffer> Source = *Device.CreateBuffer( { .Size = 256ULL, .Usage = ERHIResourceUsage::CopySource } );
            TSharedRef<IRHIBuffer> Destination =
                *Device.CreateBuffer( { .Size = 256ULL, .Usage = ERHIResourceUsage::CopyDestination } );
            TSharedRef<IRHIQueue> Queue = *Device.GetQueue( ERHIQueueType::Graphics, 0U );

            for ( UInt32 Pass = 0U; Pass < 128U; ++Pass )
            {
                LUMEN_CHECK_TRUE( List->Begin().HasValue() );
                for ( UInt32 Index = 0U; Index < 32U; ++Index )
                {
                    const UInt64 Offset = static_cast<UInt64>( Index % 16U ) * 16ULL;
                    LUMEN_CHECK_TRUE( List->CopyBuffer( Source, Destination,
                                                        { .SourceOffset = Offset, .DestinationOffset = Offset, .Size = 16ULL } )
                                          .HasValue() );
                }
                FRHIBufferTransition BufferTransition;
                BufferTransition.Buffer         = Source;
                BufferTransition.Before         = ERHIResourceState::Common;
                BufferTransition.After          = ERHIResourceState::CopySource;
                const FRHITransition Transition = BufferTransition;
                for ( UInt32 Index = 0U; Index < 4U; ++Index )
                {
                    LUMEN_CHECK_TRUE( List->Transition( TSpan<const FRHITransition>( &Transition, 1U ) ).HasValue() );
                }
                LUMEN_CHECK_TRUE( List->End().HasValue() );
                FRHISubmitDesc Submit;
                Submit.CommandLists = { List };
                Submit.Completion   = { static_cast<UInt64>( Pass ) + 1ULL };
                LUMEN_CHECK_TRUE( Queue->Submit( Submit ).HasValue() );
                LUMEN_CHECK_TRUE( Queue->Wait( Submit.Completion, 0.0 ).HasValue() );
                LUMEN_CHECK_TRUE( Allocator->Reset( Queue->GetCompletedValue() ).HasValue() );
                LUMEN_CHECK_EQ( List->GetState(), ERHICommandListState::Initial );
                LUMEN_CHECK_EQ( List->GetStream().GetCommandCount(), 0U );
                LUMEN_CHECK_EQ( List->GetStream().GetRetainedCount(), 0U );
            }
        }

    } // namespace
} // namespace RHI

} // namespace Lumen
