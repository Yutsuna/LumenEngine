/**
 * @file LumenCompilerTest.hpp
 * @brief LumenCompilerTest class definition.
 */

#include "LumenCompiler/LumenCompilerTypes.hpp"
#include "LumenCompilerTest.hpp"

namespace LumenEngine
{

TEST( FLumenCacheMetaData, RoundTripMesh )
{
    Compiler::FLumenCacheMetaData Original;
    Original.SourceHash    = 0xDEADBEEFCAFEBABEULL;
    Original.AssetType     = Compiler::EAssetType::Mesh;
    Original.CompiledAtNs  = 1234567890ULL;
    Original.BlobByteCount = 4096U;
    Original.BlockName     = "Hero";

    const TVector<Byte> Serialized = Original.Serialize();
    ASSERT_FALSE( Serialized.empty() );

    const TOptional<Compiler::FLumenCacheMetaData> Restored = Compiler::FLumenCacheMetaData::Deserialize( std::span<const Byte>( Serialized ) );

    ASSERT_TRUE( Restored.has_value() );
    EXPECT_EQ( Restored->SourceHash, Original.SourceHash );
    EXPECT_EQ( Restored->AssetType, Original.AssetType );
    EXPECT_EQ( Restored->CompiledAtNs, Original.CompiledAtNs );
    EXPECT_EQ( Restored->BlobByteCount, Original.BlobByteCount );
    EXPECT_EQ( Restored->BlockName, Original.BlockName );
}

TEST( FLumenCacheMetaData, RoundTripMaterial )
{
    Compiler::FLumenCacheMetaData Original;
    Original.SourceHash    = 0x1122334455667788ULL;
    Original.AssetType     = Compiler::EAssetType::Material;
    Original.CompiledAtNs  = 9999999999ULL;
    Original.BlobByteCount = 64U;
    Original.BlockName     = "Ground";

    const TOptional<Compiler::FLumenCacheMetaData> Restored = Compiler::FLumenCacheMetaData::Deserialize( std::span<const Byte>( Original.Serialize() ) );

    ASSERT_TRUE( Restored.has_value() );
    EXPECT_EQ( Restored->AssetType, Compiler::EAssetType::Material );
    EXPECT_EQ( Restored->BlockName, "Ground" );
}

TEST( FLumenCacheMetaData, EmptyBlockNameRoundTrip )
{
    Compiler::FLumenCacheMetaData Original;
    Original.SourceHash    = 0xABCDEF0123456789ULL;
    Original.AssetType     = Compiler::EAssetType::Mesh;
    Original.CompiledAtNs  = 0ULL;
    Original.BlobByteCount = 128U;
    Original.BlockName     = "";

    const TOptional<Compiler::FLumenCacheMetaData> Restored = Compiler::FLumenCacheMetaData::Deserialize( std::span<const Byte>( Original.Serialize() ) );

    ASSERT_TRUE( Restored.has_value() );
    EXPECT_EQ( Restored->BlockName, "" );
}

TEST( FLumenCacheMetaData, TruncatedBufferReturnsNullopt )
{
    Compiler::FLumenCacheMetaData Meta;
    Meta.SourceHash = 0x1ULL;
    Meta.BlockName  = "X";

    const TVector<Byte> Serialized = Meta.Serialize();

    const TOptional<Compiler::FLumenCacheMetaData> Restored =
        Compiler::FLumenCacheMetaData::Deserialize( std::span<const Byte>( Serialized.data(), Serialized.size() - 1 ) );

    EXPECT_FALSE( Restored.has_value() );
}

TEST( FLumenCacheMetaData, WrongMagicReturnsNullopt )
{
    Compiler::FLumenCacheMetaData Meta;
    Meta.SourceHash = 0x2ULL;
    Meta.BlockName  = "";

    TVector<Byte> Serialized = Meta.Serialize();

    Serialized[0] = 0xFF;
    Serialized[1] = 0xFF;

    const TOptional<Compiler::FLumenCacheMetaData> Restored = Compiler::FLumenCacheMetaData::Deserialize( std::span<const Byte>( Serialized ) );

    EXPECT_FALSE( Restored.has_value() );
}

TEST( FLumenCacheMetaData, WrongVersionReturnsNullopt )
{
    Compiler::FLumenCacheMetaData Meta;
    Meta.SourceHash = 0x3ULL;
    Meta.BlockName  = "";

    TVector<Byte> Serialized = Meta.Serialize();

    Serialized[4] = 0xFF;
    Serialized[5] = 0xFF;

    const TOptional<Compiler::FLumenCacheMetaData> Restored = Compiler::FLumenCacheMetaData::Deserialize( std::span<const Byte>( Serialized ) );

    EXPECT_FALSE( Restored.has_value() );
}

TEST( FLumenCacheMetaData, EmptySpanReturnsNullopt )
{
    const TOptional<Compiler::FLumenCacheMetaData> Restored = Compiler::FLumenCacheMetaData::Deserialize( std::span<const Byte>{} );

    EXPECT_FALSE( Restored.has_value() );
}

} // namespace LumenEngine
