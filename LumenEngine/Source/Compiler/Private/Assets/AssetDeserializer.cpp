/**
 * @file AssetDeserializer.cpp
 * @brief Implementation of the asset deserializer.
 */

#include "Assets/AssetDeserializer.hpp"
#include "Container/Expected.hpp"
#include "Container/Span.hpp"

#include <bit>
#include <cstring>

namespace
{

constexpr LumenEngine::USize MaxVertices = 1024UL * 1024UL;
constexpr LumenEngine::USize MaxIndices  = 4UL * 1024UL * 1024UL;

/**
 * @brief Internal helper to validate and cast a header from a blob.
 */
template <typename HeaderType>
LumenEngine::TExpected<const HeaderType *, LumenEngine::Compiler::ELumenCompilerError::Type>
ValidateHeader ( LumenEngine::TSpan<const LumenEngine::Byte> InBlob, LumenEngine::Compiler::EAssetType::Type InExpectedType ) noexcept
{
    using namespace LumenEngine::Compiler;

    if ( InBlob.size() < sizeof( FLumenBinaryHeader ) + sizeof( HeaderType ) )
    {
        return LumenEngine::MakeUnexpected( ELumenCompilerError::ReadFailed );
    }

    if ( reinterpret_cast<std::uintptr_t>( InBlob.data() ) % alignof( FLumenBinaryHeader ) != 0 )
    {
        return LumenEngine::MakeUnexpected( ELumenCompilerError::ParseFailed );
    }

    const auto *MainHeader = reinterpret_cast<const FLumenBinaryHeader *>( InBlob.data() );

    if ( MainHeader->Magic != LUMEN_ASSET_CACHE_MAGIC_NUMBER or MainHeader->AssetType != InExpectedType )
    {
        return LumenEngine::MakeUnexpected( ELumenCompilerError::ParseFailed );
    }

    return reinterpret_cast<const HeaderType *>( InBlob.data() + sizeof( FLumenBinaryHeader ) );
}

} // namespace

LumenEngine::TExpected<LumenEngine::Compiler::FDeserializedMesh, LumenEngine::Compiler::ELumenCompilerError::Type>
LumenEngine::Compiler::FAssetDeserializer::DeserializeMesh ( TSpan<const Byte> InBlob ) noexcept
{
    auto ViewResult = DeserializeMeshView( InBlob );
    LUMEN_EXPECT_VALUE( ViewResult );

    const FMeshView &View = ViewResult.value();

    FDeserializedMesh Result;
    Result.Header = View.Header;

    if ( not View.Vertices.empty() )
    {
        Result.Vertices.assign( View.Vertices.begin(), View.Vertices.end() );
    }

    if ( not View.Indices.empty() )
    {
        Result.Indices.assign( View.Indices.begin(), View.Indices.end() );
    }

    return Result;
}

LumenEngine::TExpected<LumenEngine::Compiler::FMeshView, LumenEngine::Compiler::ELumenCompilerError::Type>
LumenEngine::Compiler::FAssetDeserializer::DeserializeMeshView ( TSpan<const Byte> InBlob ) noexcept
{
    auto HeaderResult = ValidateHeader<FLumenBinaryMeshHeader>( InBlob, EAssetType::Mesh );
    LUMEN_EXPECT_VALUE( HeaderResult );

    const FLumenBinaryMeshHeader *MeshHeader = HeaderResult.value();

    if ( MeshHeader->VertexCount > MaxVertices or MeshHeader->IndexCount > MaxIndices )
    {
        return MakeUnexpected( ELumenCompilerError::ParseFailed );
    }

    const Byte *Cursor = InBlob.data() + sizeof( FLumenBinaryHeader ) + sizeof( FLumenBinaryMeshHeader );
    const Byte *End    = InBlob.data() + InBlob.size();

    FMeshView Result;
    Result.Header = *MeshHeader;

    if ( MeshHeader->VertexCount > 0 )
    {
        const USize VerticesSize = static_cast<USize>( MeshHeader->VertexCount ) * sizeof( Maths::FVertex );
        if ( Cursor + VerticesSize > End )
        {
            return MakeUnexpected( ELumenCompilerError::ReadFailed );
        }

        Result.Vertices = TSpan<const Maths::FVertex>( reinterpret_cast<const Maths::FVertex *>( Cursor ), MeshHeader->VertexCount );
        Cursor += VerticesSize;
    }

    if ( MeshHeader->IndexCount > 0 )
    {
        const USize IndicesSize = static_cast<USize>( MeshHeader->IndexCount ) * sizeof( UInt32 );
        if ( Cursor + IndicesSize > End )
        {
            return MakeUnexpected( ELumenCompilerError::ReadFailed );
        }

        Result.Indices = TSpan<const UInt32>( reinterpret_cast<const UInt32 *>( Cursor ), MeshHeader->IndexCount );
    }

    return Result;
}

LumenEngine::TExpected<LumenEngine::Compiler::FLumenBinaryMaterialHeader, LumenEngine::Compiler::ELumenCompilerError::Type>
LumenEngine::Compiler::FAssetDeserializer::DeserializeMaterial ( TSpan<const Byte> InBlob ) noexcept
{
    auto HeaderResult = ValidateHeader<FLumenBinaryMaterialHeader>( InBlob, EAssetType::Material );
    LUMEN_EXPECT_VALUE( HeaderResult );

    return *HeaderResult.value();
}
