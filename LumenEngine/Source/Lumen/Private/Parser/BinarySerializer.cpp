/**
 * @file BinarySerializer.cpp
 * @brief Implementation of the binary serializer.
 */

#include "Parser/BinarySerializer.hpp"

#include "Maths/Vertex.hpp"

#include "Types/BinaryTypes.hpp"

#include <cstring>

/**
 * Public Methods
 */

LumenEngine::TExpected<LumenEngine::TVector<LumenEngine::Byte>, LumenEngine::FString>
LumenEngine::Lumen::FBinarySerializer::SerializeMesh ( const FDLSLRootBlock *InMeshBlock )
{
    if ( InMeshBlock == nullptr )
    {
        return std::unexpected( "Null root block for mesh serialization." );
    }
    if ( InMeshBlock->BlockType != "Mesh" )
    {
        return std::unexpected( "Invalid root block type for mesh serialization. Expected 'Mesh' but got '" + InMeshBlock->BlockType + "'." );
    }

    const FDLSLNode *VerticesNode = FindProperty( InMeshBlock->Body, "Vertices" );
    const FDLSLNode *IndicesNode  = FindProperty( InMeshBlock->Body, "Indices" );
    const FDLSLNode *ConfigNode   = FindProperty( InMeshBlock->Body, "@Config" );

    if ( VerticesNode == nullptr )
    {
        return std::unexpected( "Missing Vertices list for " + InMeshBlock->Name + " mesh." );
    }
    if ( VerticesNode->Type != EDLSLNode::Type::List )
    {
        return std::unexpected( "Invalid Vertices list type for " + InMeshBlock->Name + " mesh. Got " + EDLSLNode::ToString( VerticesNode->Type ) + " instead of List." );
    }

    if ( IndicesNode == nullptr )
    {
        return std::unexpected( "Missing Indices list for " + InMeshBlock->Name + " mesh." );
    }
    if ( IndicesNode->Type != EDLSLNode::Type::List )
    {
        return std::unexpected( "Invalid Indices list type for " + InMeshBlock->Name + " mesh. Got " + EDLSLNode::ToString( IndicesNode->Type ) + " instead of List." );
    }

    if ( ConfigNode == nullptr )
    {
        return std::unexpected( "Missing @Config object for " + InMeshBlock->Name + " mesh." );
    }
    if ( ConfigNode->Type != EDLSLNode::Type::Object )
    {
        return std::unexpected( "Invalid @Config type for " + InMeshBlock->Name + " mesh. Got " + EDLSLNode::ToString( ConfigNode->Type ) + " instead of Object." );
    }

    TExpected<TVector<Maths::FVertex>, FString> VerticesResult = ExtractVertices( VerticesNode );
    LUMEN_EXPECT_VALUE( VerticesResult );

    TExpected<TVector<UInt32>, FString> IndicesResult = ExtractIndices( IndicesNode );
    LUMEN_EXPECT_VALUE( IndicesResult );

    const TVector<Maths::FVertex> &Vertices = VerticesResult.value();
    const TVector<UInt32> &Indices          = IndicesResult.value();

    FLumenBinaryMeshHeader MeshHeader{};
    MeshHeader.VertexCount = static_cast<UInt32>( Vertices.size() );
    MeshHeader.IndexCount  = static_cast<UInt32>( Indices.size() );
    ExtractMeshConfig( ConfigNode, MeshHeader );

    FLumenBinaryHeader Header{};
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
    if ( InMaterialBlock == nullptr or InMaterialBlock->BlockType != "Material" )
    {
        return std::unexpected( "Invalid root block for material serialization." );
    }

    FLumenBinaryMaterialHeader MaterialHeader{};
    const FDLSLNode *RenderStateNode = FindProperty( InMaterialBlock->Body, "@RenderState" );
    ExtractRenderState( RenderStateNode, MaterialHeader );

    FLumenBinaryHeader Header{};
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

LumenEngine::Lumen::FDLSLNode *LumenEngine::Lumen::FBinarySerializer::FindProperty ( const FDLSLNode *InObjectNode, FStringView InKey ) noexcept
{
    if ( InObjectNode == nullptr or InObjectNode->Type != EDLSLNodeType::Object )
    {
        return nullptr;
    }

    for ( FDLSLProperty *Property = InObjectNode->ObjectValue.Head; Property != nullptr; Property = Property->Next )
    {
        if ( Property->Key == InKey )
        {
            return Property->Value;
        }
    }

    return nullptr;
}

LumenEngine::TExpected<LumenEngine::TVector<LumenEngine::Maths::FVertex>, LumenEngine::FString>
LumenEngine::Lumen::FBinarySerializer::ExtractVertices ( const FDLSLNode *InVerticesListNode ) noexcept
{
    TVector<Maths::FVertex> Vertices;
    Vertices.reserve( InVerticesListNode->ListValue.Count );

    for ( FDLSLNode *VertexNode = InVerticesListNode->ListValue.Head; VertexNode != nullptr; VertexNode = VertexNode->Next )
    {
        if ( VertexNode->Type != EDLSLNodeType::Object )
        {
            return std::unexpected( "Vertex item must be an object." );
        }

        Maths::FVertex Vertex;

        const FDLSLNode *PositionNode = FindProperty( VertexNode, "Position" );
        if ( PositionNode != nullptr and PositionNode->Type == EDLSLNodeType::Vector )
        {
            Vertex.Position = Maths::FVec3f( PositionNode->VectorValue.Data[0], PositionNode->VectorValue.Data[1], PositionNode->VectorValue.Data[2] );
        }

        const FDLSLNode *NormalNode = FindProperty( VertexNode, "Normal" );
        if ( NormalNode != nullptr and NormalNode->Type == EDLSLNodeType::Vector )
        {
            Vertex.Normal = Maths::FVec3f( NormalNode->VectorValue.Data[0], NormalNode->VectorValue.Data[1], NormalNode->VectorValue.Data[2] );
        }

        const FDLSLNode *UVNode = FindProperty( VertexNode, "UV" );
        if ( UVNode != nullptr and UVNode->Type == EDLSLNodeType::Vector )
        {
            Vertex.UV = Maths::FVec2f( UVNode->VectorValue.Data[0], UVNode->VectorValue.Data[1] );
        }

        const FDLSLNode *TangentNode = FindProperty( VertexNode, "Tangent" );
        if ( TangentNode != nullptr and TangentNode->Type == EDLSLNodeType::Vector )
        {
            Vertex.Tangent = Maths::FVec3f( TangentNode->VectorValue.Data[0], TangentNode->VectorValue.Data[1], TangentNode->VectorValue.Data[2] );
        }

        Vertices.push_back( Vertex );
    }

    return Vertices;
}

LumenEngine::TExpected<LumenEngine::TVector<LumenEngine::UInt32>, LumenEngine::FString>
LumenEngine::Lumen::FBinarySerializer::ExtractIndices ( const FDLSLNode *InIndicesListNode ) noexcept
{
    TVector<UInt32> Indices;
    Indices.reserve( InIndicesListNode->ListValue.Count );

    for ( FDLSLNode *IndexNode = InIndicesListNode->ListValue.Head; IndexNode != nullptr; IndexNode = IndexNode->Next )
    {
        if ( IndexNode->Type != EDLSLNodeType::Number )
        {
            return std::unexpected( "Index item must be a number." );
        }

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

    const FDLSLNode *TopologyNode = FindProperty( InConfigNode, "Topology" );
    if ( TopologyNode != nullptr and TopologyNode->Type == EDLSLNodeType::Identifier )
    {
        if ( TopologyNode->IdentifierValue == "TriangleList" )
        {
            OutHeader.Topology = 3;
        }
    }

    const FDLSLNode *CullModeNode = FindProperty( InConfigNode, "CullMode" );
    if ( CullModeNode != nullptr and CullModeNode->Type == EDLSLNodeType::Identifier )
    {
        if ( CullModeNode->IdentifierValue == "Back" )
        {
            OutHeader.CullMode = 2;
        }
    }

    const FDLSLNode *WindingOrderNode = FindProperty( InConfigNode, "WindingOrder" );
    if ( WindingOrderNode != nullptr and WindingOrderNode->Type == EDLSLNodeType::Identifier )
    {
        if ( WindingOrderNode->IdentifierValue == "CCW" )
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

    const FDLSLNode *BlendModeNode = FindProperty( InStateNode, "BlendMode" );
    if ( BlendModeNode != nullptr and BlendModeNode->Type == EDLSLNodeType::Identifier )
    {
        OutHeader.BlendMode = ( BlendModeNode->IdentifierValue == "Opaque" ) ? 0 : 1;
    }

    const FDLSLNode *DepthTestNode = FindProperty( InStateNode, "DepthTest" );
    if ( DepthTestNode != nullptr and DepthTestNode->Type == EDLSLNodeType::Identifier )
    {
        OutHeader.DepthTest = ( DepthTestNode->IdentifierValue == "Less" ) ? 1 : 0;
    }

    const FDLSLNode *DepthWriteNode = FindProperty( InStateNode, "DepthWrite" );
    if ( DepthWriteNode != nullptr and DepthWriteNode->Type == EDLSLNodeType::Boolean )
    {
        OutHeader.DepthWrite = DepthWriteNode->BooleanValue ? 1 : 0;
    }

    const FDLSLNode *CullModeNode = FindProperty( InStateNode, "CullMode" );
    if ( CullModeNode != nullptr and CullModeNode->Type == EDLSLNodeType::Identifier )
    {
        OutHeader.CullMode = ( CullModeNode->IdentifierValue == "Back" ) ? 2 : 0;
    }

    const FDLSLNode *WireFrameNode = FindProperty( InStateNode, "WireFrame" );
    if ( WireFrameNode != nullptr and WireFrameNode->Type == EDLSLNodeType::Boolean )
    {
        OutHeader.WireFrame = WireFrameNode->BooleanValue ? 1 : 0;
    }
}
