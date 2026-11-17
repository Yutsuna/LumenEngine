/**
 * @file RHIShaderPackageCodecTests.cpp
 * @brief Tests versioned bounded shader package serialization.
 */

#include <Shaders/RHIShaderPackageCodec.hpp>
#include <Tests/UnitTest.hpp>

namespace Lumen
{
namespace
{

    /**
     * Private
     */

    namespace
    {

        FRHIShaderPackage MakePackage ()
        {
            FRHIShaderPackageEntry Entry;
            Entry.Bytecode                       = { 4U, 3U, 2U, 1U };
            Entry.Stage                          = ERHIShaderStage::Compute;
            Entry.EntryPoint                     = "MainCS";
            Entry.Format                         = ERHIShaderFormat::DXIL;
            Entry.Reflection.WorkgroupSize       = { 8U, 8U, 1U };
            Entry.Reflection.InlineConstantBytes = 32U;
            Entry.Reflection.Bindings.push_back(
                { .Slot = 5U, .Kind = ERHIShaderBindingKind::ReadWriteBuffer, .Count = 2U, .Stages = ERHIShaderStage::Compute } );
            FRHIShaderPackage Package;
            LUMEN_CHECK_TRUE( Package.AddEntry( std::move( Entry ) ).HasValue() );
            return Package;
        }

    } // namespace

    /**
     * Public
     */

    LUMEN_TEST( RHIShaderPackageCodec, RoundTripsOwnedEntriesAndReflection )
    {
        TExpected<TVector<Byte>, FRHIError> Encoded = EncodeRHIShaderPackage( MakePackage() );
        LUMEN_CHECK_TRUE( Encoded.HasValue() );
        TExpected<FRHIShaderPackage, FRHIError> Decoded = DecodeRHIShaderPackage( *Encoded );
        LUMEN_CHECK_TRUE( Decoded.HasValue() );
        LUMEN_CHECK_EQ( Decoded->GetEntries().size(), 1U );
        const FRHIShaderPackageEntry &Entry = Decoded->GetEntries()[0];
        LUMEN_CHECK_EQ( Entry.Stage, ERHIShaderStage::Compute );
        LUMEN_CHECK_EQ( Entry.EntryPoint, FString( "MainCS" ) );
        LUMEN_CHECK_EQ( Entry.Format, ERHIShaderFormat::DXIL );
        LUMEN_CHECK_EQ( Entry.Bytecode.size(), 4U );
        LUMEN_CHECK_EQ( Entry.Bytecode[0], 4U );
        LUMEN_CHECK_EQ( Entry.Reflection.WorkgroupSize[0], 8U );
        LUMEN_CHECK_EQ( Entry.Reflection.InlineConstantBytes, 32U );
        LUMEN_CHECK_EQ( Entry.Reflection.Bindings.size(), 1U );
        LUMEN_CHECK_EQ( Entry.Reflection.Bindings[0].Slot, 5U );
        LUMEN_CHECK_EQ( Entry.Reflection.Bindings[0].Count, 2U );
    }

    LUMEN_TEST( RHIShaderPackageCodec, RejectsBadHeadersTruncationAndTrailingBytes )
    {
        LUMEN_CHECK_HAS_ERROR( DecodeRHIShaderPackage( {} ) );
        TExpected<TVector<Byte>, FRHIError> Result = EncodeRHIShaderPackage( MakePackage() );
        TVector<Byte> Encoded                      = Result.ValueOr( TVector<Byte>{} );
        if ( Encoded.size() <= 12U )
        {
            LUMEN_CHECK_TRUE( Result.HasValue() );
            return;
        }
        TVector<Byte> BadMagic = Encoded;
        BadMagic[0]            = 0U;
        LUMEN_CHECK_HAS_ERROR( DecodeRHIShaderPackage( BadMagic ) );
        TVector<Byte> BadVersion = Encoded;
        BadVersion[4]            = 2U;
        LUMEN_CHECK_HAS_ERROR( DecodeRHIShaderPackage( BadVersion ) );
        TVector<Byte> Truncated = Encoded;
        Truncated.pop_back();
        LUMEN_CHECK_HAS_ERROR( DecodeRHIShaderPackage( Truncated ) );
        TVector<Byte> Trailing = Encoded;
        Trailing.push_back( 0U );
        LUMEN_CHECK_HAS_ERROR( DecodeRHIShaderPackage( Trailing ) );
    }

    LUMEN_TEST( RHIShaderPackageCodec, RejectsInvalidDecodedShaderMetadata )
    {
        TExpected<TVector<Byte>, FRHIError> Result = EncodeRHIShaderPackage( MakePackage() );
        TVector<Byte> Encoded                      = Result.ValueOr( TVector<Byte>{} );
        if ( Encoded.size() <= 12U )
        {
            LUMEN_CHECK_TRUE( Result.HasValue() );
            return;
        }
        TVector<Byte> InvalidStage = Encoded;
        InvalidStage[12]           = 0U;
        LUMEN_CHECK_HAS_ERROR( DecodeRHIShaderPackage( InvalidStage ) );
    }

    LUMEN_TEST( RHIShaderPackageCodec, RoundTripsEmptyPackage )
    {
        const FRHIShaderPackage Package;
        TExpected<TVector<Byte>, FRHIError> Encoded = EncodeRHIShaderPackage( Package );
        LUMEN_CHECK_TRUE( Encoded.HasValue() );
        TExpected<FRHIShaderPackage, FRHIError> Decoded = DecodeRHIShaderPackage( *Encoded );
        LUMEN_CHECK_TRUE( Decoded.HasValue() );
        LUMEN_CHECK_TRUE( Decoded->GetEntries().empty() );
    }

} // namespace
} // namespace Lumen
