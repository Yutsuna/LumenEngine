/**
 * @file AssetDeserializerTest.cpp
 * @brief Unit tests for the FAssetDeserializer class.
 */

#include "Assets/AssetDeserializer.hpp"
#include "LumenCompiler/LumenCompilerTypes.hpp"
#include <cstring>
#include <gtest/gtest.h>

using namespace LumenEngine;
using namespace LumenEngine::Compiler;

namespace
{

struct alignas( 16 ) FAlignedBuffer
{
    Byte Data[4096];
};

class FAssetDeserializerTest : public ::testing::Test
{
protected:

    void SetUp () override
    {
        std::memset( Buffer.Data, 0, sizeof( Buffer.Data ) );
    }

    FAlignedBuffer Buffer;
};

} // namespace

TEST_F( FAssetDeserializerTest, Test_FAssetDeserializer_DeserializeMeshView_Success )
{
    const UInt32 VertexCount = 3;
    const UInt32 IndexCount  = 3;

    FLumenBinaryHeader *MainHeader = reinterpret_cast<FLumenBinaryHeader *>( Buffer.Data );
    MainHeader->Magic              = LUMEN_ASSET_CACHE_MAGIC_NUMBER;
    MainHeader->FileVersion        = Version::Packed;
    MainHeader->AssetType          = EAssetType::Mesh;
    MainHeader->PayloadSize        = sizeof( FLumenBinaryMeshHeader ) + VertexCount * sizeof( Maths::FVertex ) + IndexCount * sizeof( UInt32 );

    FLumenBinaryMeshHeader *MeshHeader = reinterpret_cast<FLumenBinaryMeshHeader *>( Buffer.Data + sizeof( FLumenBinaryHeader ) );
    MeshHeader->VertexCount            = VertexCount;
    MeshHeader->IndexCount             = IndexCount;

    Maths::FVertex *Vertices = reinterpret_cast<Maths::FVertex *>( Buffer.Data + sizeof( FLumenBinaryHeader ) + sizeof( FLumenBinaryMeshHeader ) );
    for ( UInt32 i = 0; i < VertexCount; ++i )
    {
        Vertices[i].Position = { static_cast<Float32>( i ), 0.0f, 0.0f };
    }

    UInt32 *Indices = reinterpret_cast<UInt32 *>( reinterpret_cast<Byte *>( Vertices ) + VertexCount * sizeof( Maths::FVertex ) );
    for ( UInt32 i = 0; i < IndexCount; ++i )
    {
        Indices[i] = i;
    }

    TSpan<const Byte> Blob( Buffer.Data, sizeof( FLumenBinaryHeader ) + MainHeader->PayloadSize );
    auto Result = FAssetDeserializer::DeserializeMeshView( Blob );

    ASSERT_TRUE( Result.has_value() ) << "Deserialization failed with error: " << static_cast<int>( Result.error() );

    const FMeshView &View = Result.value();
    EXPECT_EQ( View.Header.VertexCount, VertexCount );
    EXPECT_EQ( View.Header.IndexCount, IndexCount );
    EXPECT_EQ( View.Vertices.size(), VertexCount );
    EXPECT_EQ( View.Indices.size(), IndexCount );

    for ( UInt32 i = 0; i < VertexCount; ++i )
    {
        EXPECT_EQ( View.Vertices[i].Position.X, static_cast<Float32>( i ) );
    }

    for ( UInt32 i = 0; i < IndexCount; ++i )
    {
        EXPECT_EQ( View.Indices[i], i );
    }
}

TEST_F( FAssetDeserializerTest, Test_FAssetDeserializer_DeserializeMesh_Success )
{
    const UInt32 VertexCount = 3;
    const UInt32 IndexCount  = 3;

    FLumenBinaryHeader *MainHeader = reinterpret_cast<FLumenBinaryHeader *>( Buffer.Data );
    MainHeader->Magic              = LUMEN_ASSET_CACHE_MAGIC_NUMBER;
    MainHeader->FileVersion        = Version::Packed;
    MainHeader->AssetType          = EAssetType::Mesh;
    MainHeader->PayloadSize        = sizeof( FLumenBinaryMeshHeader ) + VertexCount * sizeof( Maths::FVertex ) + IndexCount * sizeof( UInt32 );

    FLumenBinaryMeshHeader *MeshHeader = reinterpret_cast<FLumenBinaryMeshHeader *>( Buffer.Data + sizeof( FLumenBinaryHeader ) );
    MeshHeader->VertexCount            = VertexCount;
    MeshHeader->IndexCount             = IndexCount;

    TSpan<const Byte> Blob( Buffer.Data, sizeof( FLumenBinaryHeader ) + MainHeader->PayloadSize );
    auto Result = FAssetDeserializer::DeserializeMesh( Blob );

    ASSERT_TRUE( Result.has_value() );

    const FDeserializedMesh &Mesh = Result.value();
    EXPECT_EQ( Mesh.Header.VertexCount, VertexCount );
    EXPECT_EQ( Mesh.Vertices.size(), VertexCount );
    EXPECT_EQ( Mesh.Indices.size(), IndexCount );
}

TEST_F( FAssetDeserializerTest, Test_FAssetDeserializer_DeserializeMaterial_Success )
{
    FLumenBinaryHeader *MainHeader = reinterpret_cast<FLumenBinaryHeader *>( Buffer.Data );
    MainHeader->Magic              = LUMEN_ASSET_CACHE_MAGIC_NUMBER;
    MainHeader->FileVersion        = Version::Packed;
    MainHeader->AssetType          = EAssetType::Material;
    MainHeader->PayloadSize        = sizeof( FLumenBinaryMaterialHeader );

    FLumenBinaryMaterialHeader *MaterialHeader = reinterpret_cast<FLumenBinaryMaterialHeader *>( Buffer.Data + sizeof( FLumenBinaryHeader ) );
    MaterialHeader->PropertyCount              = 42;

    TSpan<const Byte> Blob( Buffer.Data, sizeof( FLumenBinaryHeader ) + MainHeader->PayloadSize );
    auto Result = FAssetDeserializer::DeserializeMaterial( Blob );

    ASSERT_TRUE( Result.has_value() );
    EXPECT_EQ( Result.value().PropertyCount, 42 );
}

TEST_F( FAssetDeserializerTest, Test_FAssetDeserializer_MalformedMagic )
{
    FLumenBinaryHeader *MainHeader = reinterpret_cast<FLumenBinaryHeader *>( Buffer.Data );
    MainHeader->Magic              = 0xBAD0BEEF;
    MainHeader->AssetType          = EAssetType::Mesh;

    TSpan<const Byte> Blob( Buffer.Data, sizeof( FLumenBinaryHeader ) + sizeof( FLumenBinaryMeshHeader ) );
    auto Result = FAssetDeserializer::DeserializeMeshView( Blob );

    EXPECT_FALSE( Result.has_value() );
    EXPECT_EQ( Result.error(), ELumenCompilerError::ParseFailed );
}

TEST_F( FAssetDeserializerTest, Test_FAssetDeserializer_WrongAssetType )
{
    FLumenBinaryHeader *MainHeader = reinterpret_cast<FLumenBinaryHeader *>( Buffer.Data );
    MainHeader->Magic              = LUMEN_ASSET_CACHE_MAGIC_NUMBER;
    MainHeader->AssetType          = EAssetType::Material;

    TSpan<const Byte> Blob( Buffer.Data, sizeof( FLumenBinaryHeader ) + sizeof( FLumenBinaryMeshHeader ) );
    auto Result = FAssetDeserializer::DeserializeMeshView( Blob );

    EXPECT_FALSE( Result.has_value() );
    EXPECT_EQ( Result.error(), ELumenCompilerError::ParseFailed );
}

TEST_F( FAssetDeserializerTest, Test_FAssetDeserializer_TruncatedBlob )
{
    FLumenBinaryHeader *MainHeader = reinterpret_cast<FLumenBinaryHeader *>( Buffer.Data );
    MainHeader->Magic              = LUMEN_ASSET_CACHE_MAGIC_NUMBER;
    MainHeader->FileVersion        = Version::Packed;
    MainHeader->AssetType          = EAssetType::Mesh;
    MainHeader->PayloadSize        = sizeof( FLumenBinaryMeshHeader ) + 100;

    FLumenBinaryMeshHeader *MeshHeader = reinterpret_cast<FLumenBinaryMeshHeader *>( Buffer.Data + sizeof( FLumenBinaryHeader ) );
    MeshHeader->VertexCount            = 10;

    TSpan<const Byte> Blob( Buffer.Data, sizeof( FLumenBinaryHeader ) + sizeof( FLumenBinaryMeshHeader ) );
    auto Result = FAssetDeserializer::DeserializeMeshView( Blob );

    EXPECT_FALSE( Result.has_value() );
    EXPECT_EQ( Result.error(), ELumenCompilerError::ReadFailed );
}

TEST_F( FAssetDeserializerTest, Test_FAssetDeserializer_ExceedLimits )
{
    FLumenBinaryHeader *MainHeader = reinterpret_cast<FLumenBinaryHeader *>( Buffer.Data );
    MainHeader->Magic              = LUMEN_ASSET_CACHE_MAGIC_NUMBER;
    MainHeader->FileVersion        = Version::Packed;
    MainHeader->AssetType          = EAssetType::Mesh;

    FLumenBinaryMeshHeader *MeshHeader = reinterpret_cast<FLumenBinaryMeshHeader *>( Buffer.Data + sizeof( FLumenBinaryHeader ) );
    MeshHeader->VertexCount            = 2 * 1024 * 1024;

    TSpan<const Byte> Blob( Buffer.Data, sizeof( FLumenBinaryHeader ) + sizeof( FLumenBinaryMeshHeader ) );
    auto Result = FAssetDeserializer::DeserializeMeshView( Blob );

    EXPECT_FALSE( Result.has_value() );
    EXPECT_EQ( Result.error(), ELumenCompilerError::ParseFailed );
}

TEST_F( FAssetDeserializerTest, Test_FAssetDeserializer_UnalignedPointer )
{
    TSpan<const Byte> Blob( Buffer.Data + 1, sizeof( FLumenBinaryHeader ) + sizeof( FLumenBinaryMeshHeader ) );
    auto Result = FAssetDeserializer::DeserializeMeshView( Blob );

    EXPECT_FALSE( Result.has_value() );
    EXPECT_EQ( Result.error(), ELumenCompilerError::ParseFailed );
}

TEST_F( FAssetDeserializerTest, Test_FAssetDeserializer_BlobTooSmallForMainHeader )
{
    TSpan<const Byte> Blob( Buffer.Data, sizeof( FLumenBinaryHeader ) - 1 );
    auto Result = FAssetDeserializer::DeserializeMeshView( Blob );

    EXPECT_FALSE( Result.has_value() );
    EXPECT_EQ( Result.error(), ELumenCompilerError::ReadFailed );
}

TEST_F( FAssetDeserializerTest, Test_FAssetDeserializer_BlobTooSmallForMeshHeader )
{
    FLumenBinaryHeader *MainHeader = reinterpret_cast<FLumenBinaryHeader *>( Buffer.Data );
    MainHeader->Magic              = LUMEN_ASSET_CACHE_MAGIC_NUMBER;
    MainHeader->AssetType          = EAssetType::Mesh;

    TSpan<const Byte> Blob( Buffer.Data, sizeof( FLumenBinaryHeader ) + sizeof( FLumenBinaryMeshHeader ) - 1 );
    auto Result = FAssetDeserializer::DeserializeMeshView( Blob );

    EXPECT_FALSE( Result.has_value() );
    EXPECT_EQ( Result.error(), ELumenCompilerError::ReadFailed );
}

TEST_F( FAssetDeserializerTest, Test_FAssetDeserializer_MeshWithZeroVertices )
{
    FLumenBinaryHeader *MainHeader = reinterpret_cast<FLumenBinaryHeader *>( Buffer.Data );
    MainHeader->Magic              = LUMEN_ASSET_CACHE_MAGIC_NUMBER;
    MainHeader->AssetType          = EAssetType::Mesh;
    MainHeader->PayloadSize        = sizeof( FLumenBinaryMeshHeader );

    FLumenBinaryMeshHeader *MeshHeader = reinterpret_cast<FLumenBinaryMeshHeader *>( Buffer.Data + sizeof( FLumenBinaryHeader ) );
    MeshHeader->VertexCount            = 0;
    MeshHeader->IndexCount             = 0;

    TSpan<const Byte> Blob( Buffer.Data, sizeof( FLumenBinaryHeader ) + sizeof( FLumenBinaryMeshHeader ) );
    auto Result = FAssetDeserializer::DeserializeMeshView( Blob );

    ASSERT_TRUE( Result.has_value() );
    EXPECT_EQ( Result.value().Vertices.size(), 0U );
    EXPECT_EQ( Result.value().Indices.size(), 0U );
}

TEST_F( FAssetDeserializerTest, Test_FAssetDeserializer_TruncatedVertexData )
{
    FLumenBinaryHeader *MainHeader = reinterpret_cast<FLumenBinaryHeader *>( Buffer.Data );
    MainHeader->Magic              = LUMEN_ASSET_CACHE_MAGIC_NUMBER;
    MainHeader->AssetType          = EAssetType::Mesh;

    FLumenBinaryMeshHeader *MeshHeader = reinterpret_cast<FLumenBinaryMeshHeader *>( Buffer.Data + sizeof( FLumenBinaryHeader ) );
    MeshHeader->VertexCount            = 10;
    MeshHeader->IndexCount             = 0;

    // Blob has header but only half of vertex data
    TSpan<const Byte> Blob( Buffer.Data, sizeof( FLumenBinaryHeader ) + sizeof( FLumenBinaryMeshHeader ) + ( 5 * sizeof( Maths::FVertex ) ) );
    auto Result = FAssetDeserializer::DeserializeMeshView( Blob );

    EXPECT_FALSE( Result.has_value() );
    EXPECT_EQ( Result.error(), ELumenCompilerError::ReadFailed );
}

TEST_F( FAssetDeserializerTest, Test_FAssetDeserializer_TruncatedIndexData )
{
    FLumenBinaryHeader *MainHeader = reinterpret_cast<FLumenBinaryHeader *>( Buffer.Data );
    MainHeader->Magic              = LUMEN_ASSET_CACHE_MAGIC_NUMBER;
    MainHeader->AssetType          = EAssetType::Mesh;

    FLumenBinaryMeshHeader *MeshHeader = reinterpret_cast<FLumenBinaryMeshHeader *>( Buffer.Data + sizeof( FLumenBinaryHeader ) );
    MeshHeader->VertexCount            = 1;
    MeshHeader->IndexCount             = 10;

    // Blob has header, full vertex data, but only half of index data
    TSpan<const Byte> Blob( Buffer.Data, sizeof( FLumenBinaryHeader ) + sizeof( FLumenBinaryMeshHeader ) + sizeof( Maths::FVertex ) + ( 5 * sizeof( UInt32 ) ) );
    auto Result = FAssetDeserializer::DeserializeMeshView( Blob );

    EXPECT_FALSE( Result.has_value() );
    EXPECT_EQ( Result.error(), ELumenCompilerError::ReadFailed );
}

TEST_F( FAssetDeserializerTest, Test_FAssetDeserializer_DeserializeMaterial_Truncated )
{
    FLumenBinaryHeader *MainHeader = reinterpret_cast<FLumenBinaryHeader *>( Buffer.Data );
    MainHeader->Magic              = LUMEN_ASSET_CACHE_MAGIC_NUMBER;
    MainHeader->AssetType          = EAssetType::Material;

    // Blob only has the main header, missing the material header
    TSpan<const Byte> Blob( Buffer.Data, sizeof( FLumenBinaryHeader ) );
    auto Result = FAssetDeserializer::DeserializeMaterial( Blob );

    EXPECT_FALSE( Result.has_value() );
    EXPECT_EQ( Result.error(), ELumenCompilerError::ReadFailed );
}
