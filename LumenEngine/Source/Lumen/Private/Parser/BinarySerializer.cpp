/**
 * @file BinarySerializer.cpp
 * @brief Implementation of the binary serializer.
 */

#include "Parser/BinarySerializer.hpp"
#include "Container/Expected.hpp"
#include "Maths/Vertex.hpp"
#include "Types/BinaryTypes.hpp"

#include <cstring>

namespace
{

inline LumenEngine::FString ToString ( LumenEngine::FStringView InView )
{
    return { InView.data(), InView.size() };
}

} // namespace

/**
 * Public Methods
 */

LumenEngine::TExpected<LumenEngine::TVector<LumenEngine::Byte>, LumenEngine::FString>
LumenEngine::Lumen::FBinarySerializer::SerializeMesh ( const FDLSLRootBlock *InMeshBlock )
{
    LUMEN_EXPECT( InMeshBlock != nullptr, "Null root block for mesh serialization." );

    if ( InMeshBlock->BlockType != "Mesh" )
    {
        return MakeUnexpected( "Invalid root block type for mesh serialization. Expected 'Mesh' but got '" + ToString( InMeshBlock->BlockType ) + "'." );
    }

    const FDLSLNode *const VerticesNode = FindProperty( InMeshBlock->Body, "Vertices" );
    const FDLSLNode *const IndicesNode  = FindProperty( InMeshBlock->Body, "Indices" );
    const FDLSLNode *const ConfigNode   = FindProperty( InMeshBlock->Body, "@Config" );

    LUMEN_EXPECT( VerticesNode != nullptr, "Missing Vertices list for " + ToString( InMeshBlock->Name ) + " mesh." );
    if ( VerticesNode->Type != EDLSLNodeType::List )
    {
        return MakeUnexpected( "Invalid Vertices list type for " + ToString( InMeshBlock->Name ) + " mesh. Got " + EDLSLNodeType::ToString( VerticesNode->Type ) +
                               " instead of List." );
    }

    LUMEN_EXPECT( IndicesNode != nullptr, "Missing Indices list for " + ToString( InMeshBlock->Name ) + " mesh." );
    if ( IndicesNode->Type != EDLSLNodeType::List )
    {
        return MakeUnexpected( "Invalid Indices list type for " + ToString( InMeshBlock->Name ) + " mesh. Got " + EDLSLNodeType::ToString( IndicesNode->Type ) +
                               " instead of List." );
    }

    LUMEN_EXPECT( ConfigNode != nullptr, "Missing @Config object for " + ToString( InMeshBlock->Name ) + " mesh." );
    if ( ConfigNode->Type != EDLSLNodeType::Object )
    {
        return MakeUnexpected( "Invalid @Config type for " + ToString( InMeshBlock->Name ) + " mesh. Got " + EDLSLNodeType::ToString( ConfigNode->Type ) +
                               " instead of Object." );
    }

    const TExpected<TVector<Maths::FVertex>, FString> VerticesResult = ExtractVertices( VerticesNode );
    LUMEN_EXPECT_VALUE( VerticesResult );

    const TExpected<TVector<UInt32>, FString> IndicesResult = ExtractIndices( IndicesNode );
    LUMEN_EXPECT_VALUE( IndicesResult );

    const TVector<Maths::FVertex> &Vertices = VerticesResult.value();
    const TVector<UInt32> &Indices          = IndicesResult.value();

    FLumenBinaryMeshHeader MeshHeader{};
    MeshHeader.VertexCount = static_cast<UInt32>( Vertices.size() );
    MeshHeader.IndexCount  = static_cast<UInt32>( Indices.size() );
    ExtractMeshConfig( ConfigNode, MeshHeader );

    FLumenBinaryHeader Header;
    Header.Magic       = LUMEN_ASSET_CACHE_MAGIC_NUMBER;
    Header.Version     = Version::Packed;
    Header.AssetType   = EAssetType::Type::Mesh;
    Header.PayloadSize = static_cast<UInt32>( sizeof( FLumenBinaryMeshHeader ) + ( Vertices.size() * sizeof( Maths::FVertex ) ) + ( Indices.size() * sizeof( UInt32 ) ) );

    TVector<Byte> BinaryBlob;
    BinaryBlob.resize( sizeof( FLumenBinaryHeader ) + Header.PayloadSize );

    Byte *Cursor = BinaryBlob.data();
    std::memcpy( Cursor, &Header, sizeof( FLumenBinaryHeader ) );
    Cursor += sizeof( FLumenBinaryHeader );

    std::memcpy( Cursor, &MeshHeader, sizeof( FLumenBinaryMeshHeader ) );
    Cursor += sizeof( FLumenBinaryMeshHeader );

    std::memcpy( Cursor, Vertices.data(), Vertices.size() * sizeof( Maths::FVertex ) );
    Cursor += Vertices.size() * sizeof( Maths::FVertex );

    std::memcpy( Cursor, Indices.data(), Indices.size() * sizeof( UInt32 ) );

    return BinaryBlob;
}

LumenEngine::TExpected<LumenEngine::TVector<LumenEngine::Byte>, LumenEngine::FString>
LumenEngine::Lumen::FBinarySerializer::SerializeMaterial ( const FDLSLRootBlock *InMaterialBlock )
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
    Header.Version     = Version::Packed;
    Header.AssetType   = EAssetType::Type::Material;
    Header.PayloadSize = sizeof( FLumenBinaryMaterialHeader );

    TVector<Byte> BinaryBlob;
    BinaryBlob.resize( sizeof( FLumenBinaryHeader ) + Header.PayloadSize );

    Byte *Cursor = BinaryBlob.data();
    std::memcpy( Cursor, &Header, sizeof( FLumenBinaryHeader ) );
    Cursor += sizeof( FLumenBinaryHeader );

    std::memcpy( Cursor, &MaterialHeader, sizeof( FLumenBinaryMaterialHeader ) );

    return BinaryBlob;
}

/**
 * Private Methods
 */

const LumenEngine::Lumen::FDLSLNode *LumenEngine::Lumen::FBinarySerializer::FindProperty ( const FDLSLNode *InObjectNode, FStringView InKey ) noexcept
{
    if ( InObjectNode == nullptr or InObjectNode->Type != EDLSLNodeType::Object )
    {
        return nullptr;
    }

    for ( const FDLSLProperty *Property = InObjectNode->ObjectValue.Head; Property != nullptr; Property = Property->Next )
    {
        if ( Property->Key == InKey )
        {
            return Property->Value;
        }
    }

    return nullptr;
}

LumenEngine::TExpected<LumenEngine::TVector<LumenEngine::Maths::FVertex>, LumenEngine::FString>
LumenEngine::Lumen::FBinarySerializer::ExtractVertices ( const FDLSLNode *InVerticesListNode )
{
    TVector<Maths::FVertex> Vertices;
    Vertices.reserve( InVerticesListNode->ListValue.Count );

    for ( const FDLSLNode *VertexNode = InVerticesListNode->ListValue.Head; VertexNode != nullptr; VertexNode = VertexNode->Next )
    {
        LUMEN_EXPECT( VertexNode->Type == EDLSLNodeType::Object, "Vertex item must be an object." );

        Maths::FVertex Vertex;

        const FDLSLNode *const PositionNode = FindProperty( VertexNode, "Position" );
        if ( PositionNode != nullptr and PositionNode->Type == EDLSLNodeType::Vector )
        {
            Vertex.Position = Maths::FVec3f( PositionNode->VectorValue.Data[0], PositionNode->VectorValue.Data[1], PositionNode->VectorValue.Data[2] );
        }

        const FDLSLNode *const NormalNode = FindProperty( VertexNode, "Normal" );
        if ( NormalNode != nullptr and NormalNode->Type == EDLSLNodeType::Vector )
        {
            Vertex.Normal = Maths::FVec3f( NormalNode->VectorValue.Data[0], NormalNode->VectorValue.Data[1], NormalNode->VectorValue.Data[2] );
        }

        const FDLSLNode *const UVNode = FindProperty( VertexNode, "UV" );
        if ( UVNode != nullptr and UVNode->Type == EDLSLNodeType::Vector )
        {
            Vertex.UV = Maths::FVec2f( UVNode->VectorValue.Data[0], UVNode->VectorValue.Data[1] );
        }

        const FDLSLNode *const TangentNode = FindProperty( VertexNode, "Tangent" );
        if ( TangentNode != nullptr and TangentNode->Type == EDLSLNodeType::Vector )
        {
            Vertex.Tangent = Maths::FVec3f( TangentNode->VectorValue.Data[0], TangentNode->VectorValue.Data[1], TangentNode->VectorValue.Data[2] );
        }

        Vertices.push_back( Vertex );
    }

    return Vertices;
}

LumenEngine::TExpected<LumenEngine::TVector<LumenEngine::UInt32>, LumenEngine::FString>
LumenEngine::Lumen::FBinarySerializer::ExtractIndices ( const FDLSLNode *InIndicesListNode )
{
    TVector<UInt32> Indices;
    Indices.reserve( InIndicesListNode->ListValue.Count );

    for ( const FDLSLNode *IndexNode = InIndicesListNode->ListValue.Head; IndexNode != nullptr; IndexNode = IndexNode->Next )
    {
        LUMEN_EXPECT( IndexNode->Type == EDLSLNodeType::Number, "Index item must be a number." );

        Indices.push_back( static_cast<UInt32>( IndexNode->NumberValue ) );
    }

    return Indices;
}

void LumenEngine::Lumen::FBinarySerializer::ExtractMeshConfig ( const FDLSLNode *InConfigNode, FLumenBinaryMeshHeader &OutHeader ) noexcept
{
    if ( InConfigNode == nullptr or InConfigNode->Type != EDLSLNodeType::Object )
    {
        return;
    }

    const FDLSLNode *const TopologyNode = FindProperty( InConfigNode, "Topology" );
    if ( TopologyNode != nullptr and TopologyNode->Type == EDLSLNodeType::Identifier )
    {
        if ( TopologyNode->GetString() == "TriangleList" )
        {
            OutHeader.Topology = 3;
        }
    }

    const FDLSLNode *const CullModeNode = FindProperty( InConfigNode, "CullMode" );
    if ( CullModeNode != nullptr and CullModeNode->Type == EDLSLNodeType::Identifier )
    {
        if ( CullModeNode->GetString() == "Back" )
        {
            OutHeader.CullMode = 2;
        }
    }

    const FDLSLNode *const WindingOrderNode = FindProperty( InConfigNode, "WindingOrder" );
    if ( WindingOrderNode != nullptr and WindingOrderNode->Type == EDLSLNodeType::Identifier )
    {
        if ( WindingOrderNode->GetString() == "CCW" )
        {
            OutHeader.WindingOrder = 1;
        }
    }
}

void LumenEngine::Lumen::FBinarySerializer::ExtractRenderState ( const FDLSLNode *InStateNode, FLumenBinaryMaterialHeader &OutHeader ) noexcept
{
    if ( InStateNode == nullptr or InStateNode->Type != EDLSLNodeType::Object )
    {
        return;
    }

    const FDLSLNode *const BlendModeNode = FindProperty( InStateNode, "BlendMode" );
    if ( BlendModeNode != nullptr and BlendModeNode->Type == EDLSLNodeType::Identifier )
    {
        OutHeader.BlendMode = ( BlendModeNode->GetString() == "Opaque" ) ? 0 : 1;
    }

    const FDLSLNode *const DepthTestNode = FindProperty( InStateNode, "DepthTest" );
    if ( DepthTestNode != nullptr and DepthTestNode->Type == EDLSLNodeType::Identifier )
    {
        OutHeader.DepthTest = ( DepthTestNode->GetString() == "Less" ) ? 1 : 0;
    }

    const FDLSLNode *const DepthWriteNode = FindProperty( InStateNode, "DepthWrite" );
    if ( DepthWriteNode != nullptr and DepthWriteNode->Type == EDLSLNodeType::Boolean )
    {
        OutHeader.DepthWrite = DepthWriteNode->BooleanValue ? 1 : 0;
    }

    const FDLSLNode *const CullModeNode = FindProperty( InStateNode, "CullMode" );
    if ( CullModeNode != nullptr and CullModeNode->Type == EDLSLNodeType::Identifier )
    {
        OutHeader.CullMode = ( CullModeNode->GetString() == "Back" ) ? 2 : 0;
    }

    const FDLSLNode *const WireFrameNode = FindProperty( InStateNode, "WireFrame" );
    if ( WireFrameNode != nullptr and WireFrameNode->Type == EDLSLNodeType::Boolean )
    {
        OutHeader.WireFrame = WireFrameNode->BooleanValue ? 1 : 0;
    }
}