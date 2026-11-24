/**
 * @file RHIValidationBindingTests.cpp
 * @brief Tests binding layout, type, usage, range, and ownership validation.
 */
#include "Validation/RHIValidationReporter.hpp"
#include "Validation/RHIValidationTestUtils.hpp"
#include <RHI/Bindings/IRHIBindingSet.hpp>
#include <Tests/UnitTest.hpp>

namespace Lumen
{

namespace RHI
{
    namespace
    {
        LUMEN_TEST( RHIValidationBinding, AcceptsValidWritesAndRejectsWrongSlotsAndTypes )
        {
            Tests::FRecordingDebugSink Sink;
            TSharedRef<IRHIDevice> Device = Tests::Validation::MakeDevice( Sink );
            FRHIBindingLayoutDesc LayoutDesc;
            LayoutDesc.Entries.push_back( { .Slot = 0U, .Type = ERHIBindingType::ReadOnlyBuffer } );
            LayoutDesc.Entries.push_back( { .Slot = 1U, .Type = ERHIBindingType::Texture } );
            TSharedRef<IRHIBindingLayout> Layout = *Device->CreateBindingLayout( LayoutDesc );
            TSharedRef<IRHIBindingSet> Set       = *Device->CreateBindingSet( Layout );
            TSharedRef<IRHIBuffer> Buffer        = Tests::Validation::MakeBuffer( Device );
            FRHIBindingWrite Write;
            Write.Slot     = 0U;
            Write.Resource = FRHIBufferBinding{ .Buffer = Buffer };
            LUMEN_CHECK_TRUE( Set->Write( TSpan<const FRHIBindingWrite>( &Write, 1U ) ).HasValue() );
            LUMEN_CHECK_TRUE( Set->IsDirty() );
            Write.Slot               = 2U;
            FRHIExpected MissingSlot = Set->Write( TSpan<const FRHIBindingWrite>( &Write, 1U ) );
            LUMEN_CHECK_HAS_ERROR( MissingSlot );
            LUMEN_CHECK_EQ( MissingSlot.Error().Code, ERHIErrorCode::ValidationFailed );
            Write.Slot             = 1U;
            FRHIExpected WrongType = Set->Write( TSpan<const FRHIBindingWrite>( &Write, 1U ) );
            LUMEN_CHECK_EQ( WrongType.Error().Code, ERHIErrorCode::ValidationFailed );
            LUMEN_CHECK_TRUE( Sink.ContainsCode( static_cast<UInt32>( ERHIValidationMessageCode::InvalidBinding ) ) );
        }

        LUMEN_TEST( RHIValidationBinding, RejectsInvalidRangesUsageAndForeignResources )
        {
            Tests::FRecordingDebugSink Sink;
            TSharedRef<IRHIDevice> Device = Tests::Validation::MakeDevice( Sink );
            FRHIBindingLayoutDesc LayoutDesc;
            LayoutDesc.Entries.push_back( { .Slot = 0U, .Type = ERHIBindingType::ReadOnlyBuffer } );
            TSharedRef<IRHIBindingLayout> Layout = *Device->CreateBindingLayout( LayoutDesc );
            TSharedRef<IRHIBindingSet> Set       = *Device->CreateBindingSet( Layout );
            TSharedRef<IRHIBuffer> Buffer        = Tests::Validation::MakeBuffer( Device );
            FRHIBindingWrite Write;
            Write.Resource = FRHIBufferBinding{ .Buffer = Buffer, .Offset = 65ULL };
            LUMEN_CHECK_EQ( Set->Write( TSpan<const FRHIBindingWrite>( &Write, 1U ) ).Error().Code,
                            ERHIErrorCode::ValidationFailed );
            TSharedRef<IRHIBuffer> WrongUsage = Tests::Validation::MakeBuffer( Device, 64ULL, ERHIResourceUsage::CopySource );
            Write.Resource                    = FRHIBufferBinding{ .Buffer = WrongUsage };
            LUMEN_CHECK_EQ( Set->Write( TSpan<const FRHIBindingWrite>( &Write, 1U ) ).Error().Code,
                            ERHIErrorCode::ValidationFailed );
            TSharedRef<IRHIDevice> OtherDevice = Tests::Validation::MakeDevice( Sink );
            TSharedRef<IRHIBuffer> Foreign     = Tests::Validation::MakeBuffer( OtherDevice );
            Write.Resource                     = FRHIBufferBinding{ .Buffer = Foreign };
            LUMEN_CHECK_EQ( Set->Write( TSpan<const FRHIBindingWrite>( &Write, 1U ) ).Error().Code,
                            ERHIErrorCode::ValidationFailed );
            LUMEN_CHECK_TRUE( Sink.ContainsCode( static_cast<UInt32>( ERHIValidationMessageCode::ForeignObject ) ) );
        }
    } // namespace
} // namespace RHI

} // namespace Lumen
