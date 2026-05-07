/**
 * @file BinarySerializer.cpp
 * @brief Implementation of the binary serializer for DLSL assets.
 */

#include "LumenCompiler/Internal/BinarySerializer.hpp"
#include "Container/Span.hpp"
#include "LumenCompiler/LumenCompilerTypes.hpp"

#include "Maths/Vertex.hpp"

#include <cstring>

namespace
{

class FBinaryWriter
{
public:

    explicit FBinaryWriter ( LumenEngine::TVector<LumenEngine::Byte> &OutBuffer ) noexcept : Buffer( OutBuffer )
    {
        /* */
    }

    template <typename Self, typename Type> auto Write ( this Self &&InSelf, const Type &InValue ) noexcept -> Self &&
    {
        const LumenEngine::USize Offset = InSelf.Buffer.size();

        InSelf.Buffer.resize( Offset + sizeof( Type ) );
        std::memcpy( InSelf.Buffer.data() + Offset, &InValue, sizeof( Type ) );

        return std::forward<Self>( InSelf );
    }

    template <typename Self, typename Type> auto WriteSpan ( this Self &&InSelf, LumenEngine::TSpan<const Type> InSpan ) noexcept -> Self &&
    {
        if ( InSpan.empty() )
        {
            return std::forward<Self>( InSelf );
        }

        const LumenEngine::USize Offset = InSelf.Buffer.size();
        const LumenEngine::USize Size   = InSpan.size_bytes();

        InSelf.Buffer.resize( Offset + Size );
        std::memcpy( InSelf.Buffer.data() + Offset, InSpan.data(), Size );
        return std::forward<Self>( InSelf );
    }

    LumenEngine::TVector<LumenEngine::Byte> &Buffer;
};

} // namespace

LumenEngine::TExpected<LumenEngine::TVector<LumenEngine::Byte>, LumenEngine::FString>
LumenEngine::Compiler::FBinarySerializer::SerializeMesh ( const FDLSLRootBlock *InMeshBlock )
{
    LUMEN_EXPECT( InMeshBlock != nullptr, "Null root block for mesh serialization." );

    if ( InMeshBlock->BlockType != "Mesh" )
    {
        return MakeUnexpected( "Invalid root block type for mesh serialization. Expected 'Mesh' but got '" + StringViewToString( InMeshBlock->BlockType ) + "'." );
    }

    const FDLSLNode *const VerticesNode = FindProperty( InMeshBlock->Body, "Vertices" );
    const FDLSLNode *const IndicesNode  = FindProperty( InMeshBlock->Body, "Indices" );
    const FDLSLNode *const ConfigNode   = FindProperty( InMeshBlock->Body, "@Config" );

    LUMEN_EXPECT( VerticesNode != nullptr, "Missing Vertices list." );
    LUMEN_EXPECT( IndicesNode != nullptr, "Missing Indices list." );

    const auto VerticesResult = ExtractVertices( VerticesNode );
    LUMEN_EXPECT_VALUE( VerticesResult );

    const auto IndicesResult = ExtractIndices( IndicesNode );
    LUMEN_EXPECT_VALUE( IndicesResult );

    const TVector<Maths::FVertex> &Vertices = VerticesResult.value();
    const TVector<UInt32> &Indices          = IndicesResult.value();

    FLumenBinaryMeshHeader MeshHeader{};
    MeshHeader.Topology     = 3U; //<< Default: TriangleList
    MeshHeader.CullMode     = 2U; //<< Default: Back
    MeshHeader.WindingOrder = 1U; //<< Default: CCW
    MeshHeader.VertexCount  = static_cast<UInt32>( Vertices.size() );
    MeshHeader.IndexCount   = static_cast<UInt32>( Indices.size() );

    if ( ConfigNode != nullptr )
    {
        ExtractMeshConfig( ConfigNode, MeshHeader );
    }

    FLumenBinaryHeader Header;
    Header.Magic       = LUMEN_ASSET_CACHE_MAGIC_NUMBER;
    Header.FileVersion = Version::Packed;
    Header.AssetType   = EAssetType::Mesh;
    Header.PayloadSize = static_cast<UInt32>( sizeof( FLumenBinaryMeshHeader ) + ( Vertices.size() * sizeof( Maths::FVertex ) ) + ( Indices.size() * sizeof( UInt32 ) ) );

    TVector<Byte> BinaryBlob;
    BinaryBlob.reserve( sizeof( FLumenBinaryHeader ) + Header.PayloadSize );

    FBinaryWriter( BinaryBlob ).Write( Header ).Write( MeshHeader ).WriteSpan( TSpan<const Maths::FVertex>( Vertices ) ).WriteSpan( TSpan<const UInt32>( Indices ) );

    return BinaryBlob;
}

LumenEngine::TExpected<LumenEngine::TVector<LumenEngine::Byte>, LumenEngine::FString>
LumenEngine::Compiler::FBinarySerializer::SerializeMaterial ( const FDLSLRootBlock *InMaterialBlock )
{
    LUMEN_EXPECT( InMaterialBlock != nullptr, "Null root block for material serialization." );

    if ( InMaterialBlock->BlockType != "Material" )
    {
        return MakeUnexpected( "Invalid root block for material serialization." );
    }

    FLumenBinaryMaterialHeader MaterialHeader{};
    const FDLSLNode *const RenderStateNode = FindProperty( InMaterialBlock->Body, "@RenderState" );
    ExtractRenderState( RenderStateNode, MaterialHeader );

    FLumenBinaryHeader Header;
    Header.Magic       = LUMEN_ASSET_CACHE_MAGIC_NUMBER;
    Header.FileVersion = Version::Packed;
    Header.AssetType   = EAssetType::Material;
    Header.PayloadSize = sizeof( FLumenBinaryMaterialHeader );

    TVector<Byte> BinaryBlob;
    BinaryBlob.reserve( sizeof( FLumenBinaryHeader ) + Header.PayloadSize );

    FBinaryWriter( BinaryBlob ).Write( Header ).Write( MaterialHeader );

    return BinaryBlob;
}
