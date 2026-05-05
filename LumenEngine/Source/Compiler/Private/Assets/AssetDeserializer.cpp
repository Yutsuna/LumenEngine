/**
 * @file AssetDeserializer.cpp
 * @brief Implementation of the asset deserializer.
 */

#include "Assets/AssetDeserializer.hpp"

#include <cstring>

namespace
{

constexpr LumenEngine::USize MaxVertices = 1024UL * 1024UL;
constexpr LumenEngine::USize MaxIndices  = 4UL * 1024UL * 1024UL;

} // namespace

LumenEngine::TOptional<LumenEngine::Compiler::FDeserializedMesh> LumenEngine::Compiler::FAssetDeserializer::DeserializeMesh ( const TVector<Byte> &InBlob ) noexcept
{
    if ( InBlob.size() < sizeof( FLumenBinaryHeader ) + sizeof( FLumenBinaryMeshHeader ) )
    {
        return std::nullopt;
    }

    const Byte *Cursor = InBlob.data();
    const Byte *End    = InBlob.data() + InBlob.size();

    FLumenBinaryHeader MainHeader;
    std::memcpy( &MainHeader, Cursor, sizeof( FLumenBinaryHeader ) );
    Cursor += sizeof( FLumenBinaryHeader );

    if ( MainHeader.Magic != LUMEN_ASSET_CACHE_MAGIC_NUMBER or MainHeader.AssetType != EAssetType::Mesh )
    {
        return std::nullopt;
    }

    FDeserializedMesh Result;
    std::memcpy( &Result.Header, Cursor, sizeof( FLumenBinaryMeshHeader ) );
    Cursor += sizeof( FLumenBinaryMeshHeader );

    if ( Result.Header.VertexCount > 0 )
    {
        if ( Result.Header.VertexCount > MaxVertices )
        {
            return std::nullopt;
        }

        const USize VerticesSize = static_cast<USize>( Result.Header.VertexCount ) * sizeof( Maths::FVertex );
        if ( Cursor + VerticesSize > End )
        {
            return std::nullopt;
        }

        Result.Vertices.resize( Result.Header.VertexCount );
        std::memcpy( Result.Vertices.data(), Cursor, VerticesSize );
        Cursor += VerticesSize;
    }

    if ( Result.Header.IndexCount > 0 )
    {
        if ( Result.Header.IndexCount > MaxIndices )
        {
            return std::nullopt;
        }

        const USize IndicesSize = static_cast<USize>( Result.Header.IndexCount ) * sizeof( UInt32 );
        if ( Cursor + IndicesSize > End )
        {
            return std::nullopt;
        }

        Result.Indices.resize( Result.Header.IndexCount );
        std::memcpy( Result.Indices.data(), Cursor, IndicesSize );
    }

    return Result;
}

LumenEngine::TOptional<LumenEngine::Compiler::FLumenBinaryMaterialHeader>
LumenEngine::Compiler::FAssetDeserializer::DeserializeMaterial ( const TVector<Byte> &InBlob ) noexcept
{
    if ( InBlob.size() < sizeof( FLumenBinaryHeader ) + sizeof( FLumenBinaryMaterialHeader ) )
    {
        return std::nullopt;
    }

    const Byte *Cursor = InBlob.data();

    FLumenBinaryHeader MainHeader;
    std::memcpy( &MainHeader, Cursor, sizeof( FLumenBinaryHeader ) );
    Cursor += sizeof( FLumenBinaryHeader );

    if ( MainHeader.Magic != LUMEN_ASSET_CACHE_MAGIC_NUMBER or MainHeader.AssetType != EAssetType::Material )
    {
        return std::nullopt;
    }

    FLumenBinaryMaterialHeader Result;
    std::memcpy( &Result, Cursor, sizeof( FLumenBinaryMaterialHeader ) );

    return Result;
}
