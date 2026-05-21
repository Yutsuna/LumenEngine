/**
 * @file AssetDeserializerTest.cpp
 * @brief Unit tests for the FAssetDeserializer class.
 */

#include "Assets/AssetDeserializer.hpp"
#include "LumenCompiler/LumenCompilerTypes.hpp"

#include <cstring>
#include <gtest/gtest.h>

namespace
{

struct alignas( 16 ) FAlignedBuffer
{
    LumenEngine::Byte Data[4096];
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
    const LumenEngine::UInt32 VertexCount = 3;
    const LumenEngine::UInt32 IndexCount  = 3;

    LumenEngine::Compiler::FLumenBinaryHeader *MainHeader = reinterpret_cast<LumenEngine::Compiler::FLumenBinaryHeader *>( Buffer.Data );
    MainHeader->Magic                                     = LUMEN_ASSET_CACHE_MAGIC_NUMBER;
    MainHeader->FileVersion                               = LumenEngine::Version::Packed;
    MainHeader->AssetType                                 = LumenEngine::Compiler::EAssetType::Mesh;
    MainHeader->PayloadSize = sizeof( LumenEngine::Compiler::FLumenBinaryMeshHeader ) + ( VertexCount * sizeof( LumenEngine::Maths::FVertex ) ) +
                              ( IndexCount * sizeof( LumenEngine::UInt32 ) );

    LumenEngine::Compiler::FLumenBinaryMeshHeader *MeshHeader =
        reinterpret_cast<LumenEngine::Compiler::FLumenBinaryMeshHeader *>( Buffer.Data + sizeof( LumenEngine::Compiler::FLumenBinaryHeader ) );
    MeshHeader->VertexCount = VertexCount;
    MeshHeader->IndexCount  = IndexCount;

    LumenEngine::Maths::FVertex *Vertices = reinterpret_cast<LumenEngine::Maths::FVertex *>( Buffer.Data + sizeof( LumenEngine::Compiler::FLumenBinaryHeader ) +
                                                                                             sizeof( LumenEngine::Compiler::FLumenBinaryMeshHeader ) );
    for ( LumenEngine::UInt32 Index = 0; Index < VertexCount; ++Index )
    {
        Vertices[Index].Position = { static_cast<LumenEngine::Float32>( Index ), 0.0F, 0.0F };
    }

    LumenEngine::UInt32 *Indices =
        reinterpret_cast<LumenEngine::UInt32 *>( reinterpret_cast<LumenEngine::Byte *>( Vertices ) + ( VertexCount * sizeof( LumenEngine::Maths::FVertex ) ) );
    for ( LumenEngine::UInt32 Index = 0; Index < IndexCount; ++Index )
    {
        Indices[Index] = Index;
    }

    LumenEngine::TSpan<const LumenEngine::Byte> Blob( Buffer.Data, sizeof( LumenEngine::Compiler::FLumenBinaryHeader ) + MainHeader->PayloadSize );
    auto Result = LumenEngine::Engine::FAssetDeserializer::DeserializeMeshView( Blob );

    ASSERT_TRUE( Result.has_value() ) << "Deserialization failed with error: " << static_cast<int>( Result.error() );

    const LumenEngine::Engine::FMeshView &View = Result.value();
    EXPECT_EQ( View.Header.VertexCount, VertexCount );
    EXPECT_EQ( View.Header.IndexCount, IndexCount );
    EXPECT_EQ( View.Vertices.size(), VertexCount );
    EXPECT_EQ( View.Indices.size(), IndexCount );

    for ( LumenEngine::UInt32 Index = 0; Index < VertexCount; ++Index )
    {
        EXPECT_EQ( View.Vertices[Index].Position.X, static_cast<LumenEngine::Float32>( Index ) );
    }

    for ( LumenEngine::UInt32 Index = 0; Index < IndexCount; ++Index )
    {
        EXPECT_EQ( View.Indices[Index], Index );
    }
}

TEST_F( FAssetDeserializerTest, Test_FAssetDeserializer_DeserializeMesh_Success )
{
    const LumenEngine::UInt32 VertexCount = 3;
    const LumenEngine::UInt32 IndexCount  = 3;

    LumenEngine::Compiler::FLumenBinaryHeader *MainHeader = reinterpret_cast<LumenEngine::Compiler::FLumenBinaryHeader *>( Buffer.Data );
    MainHeader->Magic                                     = LUMEN_ASSET_CACHE_MAGIC_NUMBER;
    MainHeader->FileVersion                               = LumenEngine::Version::Packed;
    MainHeader->AssetType                                 = LumenEngine::Compiler::EAssetType::Mesh;
    MainHeader->PayloadSize = sizeof( LumenEngine::Compiler::FLumenBinaryMeshHeader ) + ( VertexCount * sizeof( LumenEngine::Maths::FVertex ) ) +
                              ( IndexCount * sizeof( LumenEngine::UInt32 ) );

    LumenEngine::Compiler::FLumenBinaryMeshHeader *MeshHeader =
        reinterpret_cast<LumenEngine::Compiler::FLumenBinaryMeshHeader *>( Buffer.Data + sizeof( LumenEngine::Compiler::FLumenBinaryHeader ) );
    MeshHeader->VertexCount = VertexCount;
    MeshHeader->IndexCount  = IndexCount;

    LumenEngine::TSpan<const LumenEngine::Byte> Blob( Buffer.Data, sizeof( LumenEngine::Compiler::FLumenBinaryHeader ) + MainHeader->PayloadSize );
    auto Result = LumenEngine::Engine::FAssetDeserializer::DeserializeMesh( Blob );

    ASSERT_TRUE( Result.has_value() );

    const LumenEngine::Engine::FDeserializedMesh &Mesh = Result.value();
    EXPECT_EQ( Mesh.Header.VertexCount, VertexCount );
    EXPECT_EQ( Mesh.Vertices.size(), VertexCount );
    EXPECT_EQ( Mesh.Indices.size(), IndexCount );
}

TEST_F( FAssetDeserializerTest, Test_FAssetDeserializer_DeserializeMaterial_Success )
{
    LumenEngine::Compiler::FLumenBinaryHeader *MainHeader = reinterpret_cast<LumenEngine::Compiler::FLumenBinaryHeader *>( Buffer.Data );
    MainHeader->Magic                                     = LUMEN_ASSET_CACHE_MAGIC_NUMBER;
    MainHeader->FileVersion                               = LumenEngine::Version::Packed;
    MainHeader->AssetType                                 = LumenEngine::Compiler::EAssetType::Material;
    MainHeader->PayloadSize                               = sizeof( LumenEngine::Compiler::FLumenBinaryMaterialHeader );

    LumenEngine::Compiler::FLumenBinaryMaterialHeader *MaterialHeader =
        reinterpret_cast<LumenEngine::Compiler::FLumenBinaryMaterialHeader *>( Buffer.Data + sizeof( LumenEngine::Compiler::FLumenBinaryHeader ) );
    MaterialHeader->PropertyCount = 42;

    LumenEngine::TSpan<const LumenEngine::Byte> Blob( Buffer.Data, sizeof( LumenEngine::Compiler::FLumenBinaryHeader ) + MainHeader->PayloadSize );
    auto Result = LumenEngine::Engine::FAssetDeserializer::DeserializeMaterial( Blob );

    ASSERT_TRUE( Result.has_value() );
    EXPECT_EQ( Result.value().PropertyCount, 42 );
}

TEST_F( FAssetDeserializerTest, Test_FAssetDeserializer_MalformedMagic )
{
    LumenEngine::Compiler::FLumenBinaryHeader *MainHeader = reinterpret_cast<LumenEngine::Compiler::FLumenBinaryHeader *>( Buffer.Data );
    MainHeader->Magic                                     = 0xBAD0BEEF;
    MainHeader->AssetType                                 = LumenEngine::Compiler::EAssetType::Mesh;

    LumenEngine::TSpan<const LumenEngine::Byte> Blob( Buffer.Data,
                                                      sizeof( LumenEngine::Compiler::FLumenBinaryHeader ) + sizeof( LumenEngine::Compiler::FLumenBinaryMeshHeader ) );
    auto Result = LumenEngine::Engine::FAssetDeserializer::DeserializeMeshView( Blob );

    EXPECT_FALSE( Result.has_value() );
    EXPECT_EQ( Result.error(), LumenEngine::Compiler::ELumenCompilerError::ParseFailed );
}

TEST_F( FAssetDeserializerTest, Test_FAssetDeserializer_WrongAssetType )
{
    LumenEngine::Compiler::FLumenBinaryHeader *MainHeader = reinterpret_cast<LumenEngine::Compiler::FLumenBinaryHeader *>( Buffer.Data );
    MainHeader->Magic                                     = LUMEN_ASSET_CACHE_MAGIC_NUMBER;
    MainHeader->AssetType                                 = LumenEngine::Compiler::EAssetType::Material;

    LumenEngine::TSpan<const LumenEngine::Byte> Blob( Buffer.Data,
                                                      sizeof( LumenEngine::Compiler::FLumenBinaryHeader ) + sizeof( LumenEngine::Compiler::FLumenBinaryMeshHeader ) );
    auto Result = LumenEngine::Engine::FAssetDeserializer::DeserializeMeshView( Blob );

    EXPECT_FALSE( Result.has_value() );
    EXPECT_EQ( Result.error(), LumenEngine::Compiler::ELumenCompilerError::ParseFailed );
}

TEST_F( FAssetDeserializerTest, Test_FAssetDeserializer_TruncatedBlob )
{
    LumenEngine::Compiler::FLumenBinaryHeader *MainHeader = reinterpret_cast<LumenEngine::Compiler::FLumenBinaryHeader *>( Buffer.Data );
    MainHeader->Magic                                     = LUMEN_ASSET_CACHE_MAGIC_NUMBER;
    MainHeader->FileVersion                               = LumenEngine::Version::Packed;
    MainHeader->AssetType                                 = LumenEngine::Compiler::EAssetType::Mesh;
    MainHeader->PayloadSize                               = sizeof( LumenEngine::Compiler::FLumenBinaryMeshHeader ) + 100;

    LumenEngine::Compiler::FLumenBinaryMeshHeader *MeshHeader =
        reinterpret_cast<LumenEngine::Compiler::FLumenBinaryMeshHeader *>( Buffer.Data + sizeof( LumenEngine::Compiler::FLumenBinaryHeader ) );
    MeshHeader->VertexCount = 10;

    LumenEngine::TSpan<const LumenEngine::Byte> Blob( Buffer.Data,
                                                      sizeof( LumenEngine::Compiler::FLumenBinaryHeader ) + sizeof( LumenEngine::Compiler::FLumenBinaryMeshHeader ) );
    auto Result = LumenEngine::Engine::FAssetDeserializer::DeserializeMeshView( Blob );

    EXPECT_FALSE( Result.has_value() );
    EXPECT_EQ( Result.error(), LumenEngine::Compiler::ELumenCompilerError::ReadFailed );
}

TEST_F( FAssetDeserializerTest, Test_FAssetDeserializer_ExceedLimits )
{
    LumenEngine::Compiler::FLumenBinaryHeader *MainHeader = reinterpret_cast<LumenEngine::Compiler::FLumenBinaryHeader *>( Buffer.Data );
    MainHeader->Magic                                     = LUMEN_ASSET_CACHE_MAGIC_NUMBER;
    MainHeader->FileVersion                               = LumenEngine::Version::Packed;
    MainHeader->AssetType                                 = LumenEngine::Compiler::EAssetType::Mesh;

    LumenEngine::Compiler::FLumenBinaryMeshHeader *MeshHeader =
        reinterpret_cast<LumenEngine::Compiler::FLumenBinaryMeshHeader *>( Buffer.Data + sizeof( LumenEngine::Compiler::FLumenBinaryHeader ) );
    MeshHeader->VertexCount = 2 * 1024 * 1024;

    LumenEngine::TSpan<const LumenEngine::Byte> Blob( Buffer.Data,
                                                      sizeof( LumenEngine::Compiler::FLumenBinaryHeader ) + sizeof( LumenEngine::Compiler::FLumenBinaryMeshHeader ) );
    auto Result = LumenEngine::Engine::FAssetDeserializer::DeserializeMeshView( Blob );

    EXPECT_FALSE( Result.has_value() );
    EXPECT_EQ( Result.error(), LumenEngine::Compiler::ELumenCompilerError::ParseFailed );
}

TEST_F( FAssetDeserializerTest, Test_FAssetDeserializer_UnalignedPointer )
{
    LumenEngine::TSpan<const LumenEngine::Byte> Blob( Buffer.Data + 1,
                                                      sizeof( LumenEngine::Compiler::FLumenBinaryHeader ) + sizeof( LumenEngine::Compiler::FLumenBinaryMeshHeader ) );
    auto Result = LumenEngine::Engine::FAssetDeserializer::DeserializeMeshView( Blob );

    EXPECT_FALSE( Result.has_value() );
    EXPECT_EQ( Result.error(), LumenEngine::Compiler::ELumenCompilerError::ParseFailed );
}

TEST_F( FAssetDeserializerTest, Test_FAssetDeserializer_BlobTooSmallForMainHeader )
{
    LumenEngine::TSpan<const LumenEngine::Byte> Blob( Buffer.Data, sizeof( LumenEngine::Compiler::FLumenBinaryHeader ) - 1 );
    auto Result = LumenEngine::Engine::FAssetDeserializer::DeserializeMeshView( Blob );

    EXPECT_FALSE( Result.has_value() );
    EXPECT_EQ( Result.error(), LumenEngine::Compiler::ELumenCompilerError::ReadFailed );
}

TEST_F( FAssetDeserializerTest, Test_FAssetDeserializer_BlobTooSmallForMeshHeader )
{
    LumenEngine::Compiler::FLumenBinaryHeader *MainHeader = reinterpret_cast<LumenEngine::Compiler::FLumenBinaryHeader *>( Buffer.Data );
    MainHeader->Magic                                     = LUMEN_ASSET_CACHE_MAGIC_NUMBER;
    MainHeader->AssetType                                 = LumenEngine::Compiler::EAssetType::Mesh;

    LumenEngine::TSpan<const LumenEngine::Byte> Blob( Buffer.Data,
                                                      sizeof( LumenEngine::Compiler::FLumenBinaryHeader ) + sizeof( LumenEngine::Compiler::FLumenBinaryMeshHeader ) - 1 );
    auto Result = LumenEngine::Engine::FAssetDeserializer::DeserializeMeshView( Blob );

    EXPECT_FALSE( Result.has_value() );
    EXPECT_EQ( Result.error(), LumenEngine::Compiler::ELumenCompilerError::ReadFailed );
}

TEST_F( FAssetDeserializerTest, Test_FAssetDeserializer_MeshWithZeroVertices )
{
    LumenEngine::Compiler::FLumenBinaryHeader *MainHeader = reinterpret_cast<LumenEngine::Compiler::FLumenBinaryHeader *>( Buffer.Data );
    MainHeader->Magic                                     = LUMEN_ASSET_CACHE_MAGIC_NUMBER;
    MainHeader->AssetType                                 = LumenEngine::Compiler::EAssetType::Mesh;
    MainHeader->PayloadSize                               = sizeof( LumenEngine::Compiler::FLumenBinaryMeshHeader );

    LumenEngine::Compiler::FLumenBinaryMeshHeader *MeshHeader =
        reinterpret_cast<LumenEngine::Compiler::FLumenBinaryMeshHeader *>( Buffer.Data + sizeof( LumenEngine::Compiler::FLumenBinaryHeader ) );
    MeshHeader->VertexCount = 0;
    MeshHeader->IndexCount  = 0;

    LumenEngine::TSpan<const LumenEngine::Byte> Blob( Buffer.Data,
                                                      sizeof( LumenEngine::Compiler::FLumenBinaryHeader ) + sizeof( LumenEngine::Compiler::FLumenBinaryMeshHeader ) );
    auto Result = LumenEngine::Engine::FAssetDeserializer::DeserializeMeshView( Blob );

    ASSERT_TRUE( Result.has_value() );
    EXPECT_EQ( Result.value().Vertices.size(), 0U );
    EXPECT_EQ( Result.value().Indices.size(), 0U );
}

TEST_F( FAssetDeserializerTest, Test_FAssetDeserializer_TruncatedVertexData )
{
    LumenEngine::Compiler::FLumenBinaryHeader *MainHeader = reinterpret_cast<LumenEngine::Compiler::FLumenBinaryHeader *>( Buffer.Data );
    MainHeader->Magic                                     = LUMEN_ASSET_CACHE_MAGIC_NUMBER;
    MainHeader->AssetType                                 = LumenEngine::Compiler::EAssetType::Mesh;

    LumenEngine::Compiler::FLumenBinaryMeshHeader *MeshHeader =
        reinterpret_cast<LumenEngine::Compiler::FLumenBinaryMeshHeader *>( Buffer.Data + sizeof( LumenEngine::Compiler::FLumenBinaryHeader ) );
    MeshHeader->VertexCount = 10;
    MeshHeader->IndexCount  = 0;

    LumenEngine::TSpan<const LumenEngine::Byte> Blob( Buffer.Data, sizeof( LumenEngine::Compiler::FLumenBinaryHeader ) +
                                                                       sizeof( LumenEngine::Compiler::FLumenBinaryMeshHeader ) +
                                                                       ( 5 * sizeof( LumenEngine::Maths::FVertex ) ) );
    auto Result = LumenEngine::Engine::FAssetDeserializer::DeserializeMeshView( Blob );

    EXPECT_FALSE( Result.has_value() );
    EXPECT_EQ( Result.error(), LumenEngine::Compiler::ELumenCompilerError::ReadFailed );
}

TEST_F( FAssetDeserializerTest, Test_FAssetDeserializer_TruncatedIndexData )
{
    LumenEngine::Compiler::FLumenBinaryHeader *MainHeader = reinterpret_cast<LumenEngine::Compiler::FLumenBinaryHeader *>( Buffer.Data );
    MainHeader->Magic                                     = LUMEN_ASSET_CACHE_MAGIC_NUMBER;
    MainHeader->AssetType                                 = LumenEngine::Compiler::EAssetType::Mesh;

    LumenEngine::Compiler::FLumenBinaryMeshHeader *MeshHeader =
        reinterpret_cast<LumenEngine::Compiler::FLumenBinaryMeshHeader *>( Buffer.Data + sizeof( LumenEngine::Compiler::FLumenBinaryHeader ) );
    MeshHeader->VertexCount = 1;
    MeshHeader->IndexCount  = 10;

    LumenEngine::TSpan<const LumenEngine::Byte> Blob( Buffer.Data, sizeof( LumenEngine::Compiler::FLumenBinaryHeader ) +
                                                                       sizeof( LumenEngine::Compiler::FLumenBinaryMeshHeader ) + sizeof( LumenEngine::Maths::FVertex ) +
                                                                       ( 5 * sizeof( LumenEngine::UInt32 ) ) );
    auto Result = LumenEngine::Engine::FAssetDeserializer::DeserializeMeshView( Blob );

    EXPECT_FALSE( Result.has_value() );
    EXPECT_EQ( Result.error(), LumenEngine::Compiler::ELumenCompilerError::ReadFailed );
}

TEST_F( FAssetDeserializerTest, Test_FAssetDeserializer_DeserializeMaterial_Truncated )
{
    LumenEngine::Compiler::FLumenBinaryHeader *MainHeader = reinterpret_cast<LumenEngine::Compiler::FLumenBinaryHeader *>( Buffer.Data );
    MainHeader->Magic                                     = LUMEN_ASSET_CACHE_MAGIC_NUMBER;
    MainHeader->AssetType                                 = LumenEngine::Compiler::EAssetType::Material;

    LumenEngine::TSpan<const LumenEngine::Byte> Blob( Buffer.Data, sizeof( LumenEngine::Compiler::FLumenBinaryHeader ) );
    auto Result = LumenEngine::Engine::FAssetDeserializer::DeserializeMaterial( Blob );

    EXPECT_FALSE( Result.has_value() );
    EXPECT_EQ( Result.error(), LumenEngine::Compiler::ELumenCompilerError::ReadFailed );
}
