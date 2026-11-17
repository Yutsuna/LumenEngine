/**
 * @file RHIShaderPackageCodecFuzzTests.cpp
 * @brief Fuzzes bounded shader package decoding with deterministic byte mutations.
 */

#include <Shaders/RHIShaderPackageCodec.hpp>
#include <Tests/UnitTest.hpp>
#include <Utilities/RHITestRandom.hpp>

namespace Lumen
{
namespace
{

    TVector<Byte> MakeEncodedPackage ()
    {
        FRHIShaderPackageEntry Entry;
        Entry.Bytecode                 = { 4U, 3U, 2U, 1U };
        Entry.Stage                    = ERHIShaderStage::Compute;
        Entry.EntryPoint               = "MainCS";
        Entry.Format                   = ERHIShaderFormat::BackendBytecode;
        Entry.Reflection.WorkgroupSize = { 8U, 8U, 1U };
        Entry.Reflection.Bindings.push_back(
            { .Slot = 5U, .Kind = ERHIShaderBindingKind::ReadWriteBuffer, .Count = 2U, .Stages = ERHIShaderStage::Compute } );
        FRHIShaderPackage Package;
        LUMEN_CHECK_TRUE( Package.AddEntry( std::move( Entry ) ).HasValue() );
        TExpected<TVector<Byte>, FRHIError> Encoded = EncodeRHIShaderPackage( Package );
        LUMEN_CHECK_TRUE( Encoded.HasValue() );
        return *Encoded;
    }

    void CheckDecodeResult ( const TExpected<FRHIShaderPackage, FRHIError> &Result )
    {
        if ( Result )
        {
            const TExpected<TVector<Byte>, FRHIError> Encoded = EncodeRHIShaderPackage( *Result );
            LUMEN_CHECK_TRUE( Encoded.HasValue() );
            LUMEN_CHECK_TRUE( DecodeRHIShaderPackage( *Encoded ).HasValue() );
            return;
        }
        LUMEN_CHECK_TRUE( Result.Error().Code == ERHIErrorCode::CorruptedData or
                          Result.Error().Code == ERHIErrorCode::InvalidArgument );
        LUMEN_CHECK_EQ( Result.Error().Condition, ERHIErrorCondition::Shader );
    }

    LUMEN_TEST( RHIShaderPackageCodecFuzz, EveryTruncationAndIncoherentLengthIsRejected )
    {
        const TVector<Byte> Encoded = MakeEncodedPackage();
        LUMEN_CHECK_TRUE( Encoded.size() > 52U );
        for ( USize Size = 0U; Size < Encoded.size(); ++Size )
        {
            const TExpected<FRHIShaderPackage, FRHIError> Result =
                DecodeRHIShaderPackage( TSpan<const Byte>( Encoded.data(), Size ) );
            LUMEN_CHECK_HAS_ERROR( Result );
            LUMEN_CHECK_EQ( Result.Error().Code, ERHIErrorCode::CorruptedData );
        }

        TVector<Byte> UnknownVersion = Encoded;
        UnknownVersion[4]            = 0xFFU;
        LUMEN_CHECK_HAS_ERROR( DecodeRHIShaderPackage( UnknownVersion ) );

        TVector<Byte> BytecodeLength = Encoded;
        for ( USize Index = 14U; Index < 18U; ++Index )
        {
            BytecodeLength[Index] = 0xFFU;
        }
        LUMEN_CHECK_HAS_ERROR( DecodeRHIShaderPackage( BytecodeLength ) );

        TVector<Byte> BindingCount = Encoded;
        for ( USize Index = 48U; Index < 52U; ++Index )
        {
            BindingCount[Index] = 0xFFU;
        }
        LUMEN_CHECK_HAS_ERROR( DecodeRHIShaderPackage( BindingCount ) );
    }

    LUMEN_TEST( RHIShaderPackageCodecFuzz, ArbitraryBytesNeverEscapeInputBounds )
    {
        Tests::FRHITestRandom Random( 0x4C52484942595445ULL );
        for ( UInt32 Iteration = 0U; Iteration < 512U; ++Iteration )
        {
            TVector<Byte> Bytes( static_cast<USize>( Random.Next( 257ULL ) ) );
            for ( Byte &Value : Bytes )
            {
                Value = static_cast<Byte>( Random.Next( 256ULL ) );
            }
            CheckDecodeResult( DecodeRHIShaderPackage( Bytes ) );
        }
    }

    LUMEN_TEST( RHIShaderPackageCodecFuzz, StructuredMutationsEitherFailOrRemainRoundTrippable )
    {
        const TVector<Byte> Encoded = MakeEncodedPackage();
        Tests::FRHITestRandom Random( 0x4C5248494D555441ULL );
        for ( UInt32 Iteration = 0U; Iteration < 512U; ++Iteration )
        {
            TVector<Byte> Mutated     = Encoded;
            const USize MutationCount = static_cast<USize>( Random.Next( 4ULL ) + 1ULL );
            for ( USize Mutation = 0U; Mutation < MutationCount; ++Mutation )
            {
                const USize Index = static_cast<USize>( Random.Next( Mutated.size() ) );
                const Byte Mask   = static_cast<Byte>( Random.Next( 255ULL ) + 1ULL );
                Mutated[Index]    = static_cast<Byte>( Mutated[Index] ^ Mask );
            }
            CheckDecodeResult( DecodeRHIShaderPackage( Mutated ) );
        }
    }

} // namespace
} // namespace Lumen
