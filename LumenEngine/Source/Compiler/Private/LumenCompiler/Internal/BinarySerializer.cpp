/**
 * @file BinarySerializer.cpp
 * @brief Implementation of the binary serializer for DLSL assets.
 */

#include "LumenCompiler/Internal/BinarySerializer.hpp"
#include "Container/Span.hpp"
#include "LumenCompiler/LumenCompilerTypes.hpp"

#include "Maths/Vertex.hpp"

#include <cmath>
#include <cstring>
#include <limits>

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

const LumenEngine::Compiler::FDLSLNode *LumenEngine::Compiler::FBinarySerializer::FindProperty ( const FDLSLNode *InObjectNode, const FStringView InKey ) noexcept
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
LumenEngine::Compiler::FBinarySerializer::ExtractVertices ( const FDLSLNode *InVerticesListNode )
{
    LUMEN_EXPECT( InVerticesListNode != nullptr, "Vertices node is null." );
    LUMEN_EXPECT( InVerticesListNode->Type == EDLSLNodeType::List, "Vertices must be a list." );

    TVector<Maths::FVertex> Vertices;
    Vertices.reserve( InVerticesListNode->ListValue.Count );

    const auto ReadVector = [] ( const FDLSLNode *InVectorNode, const USize InExpectedCount, const FStringView InFieldName ) -> TExpected<const Float32 *, FString>
    {
        if ( InVectorNode == nullptr )
        {
            return MakeUnexpected( "Missing vertex field: '" + StringViewToString( InFieldName ) + "'." );
        }

        if ( InVectorNode->Type != EDLSLNodeType::Vector )
        {
            return MakeUnexpected( "Vertex field '" + StringViewToString( InFieldName ) + "' must be a vector." );
        }

        if ( InVectorNode->VectorValue.Count < InExpectedCount )
        {
            return MakeUnexpected( "Vertex field '" + StringViewToString( InFieldName ) + "' has insufficient components." );
        }

        return InVectorNode->VectorValue.Data;
    };

    for ( const FDLSLNode *VertexNode = InVerticesListNode->ListValue.Head; VertexNode != nullptr; VertexNode = VertexNode->Next )
    {
        if ( VertexNode->Type != EDLSLNodeType::Object )
        {
            return MakeUnexpected( "Each vertex entry must be an object." );
        }

        const FDLSLNode *const PositionNode = FindProperty( VertexNode, "Position" );
        const FDLSLNode *const NormalNode   = FindProperty( VertexNode, "Normal" );
        const FDLSLNode *const UVNode       = FindProperty( VertexNode, "UV" );
        const FDLSLNode *const TangentNode  = FindProperty( VertexNode, "Tangent" );

        const auto PositionDataResult = ReadVector( PositionNode, 3U, "Position" );
        LUMEN_EXPECT_VALUE( PositionDataResult );

        Maths::FVertex Vertex{};
        const Float32 *const PositionData = PositionDataResult.value();
        Vertex.Position                   = { PositionData[0], PositionData[1], PositionData[2] };

        if ( NormalNode != nullptr )
        {
            const auto NormalDataResult = ReadVector( NormalNode, 3U, "Normal" );
            LUMEN_EXPECT_VALUE( NormalDataResult );

            const Float32 *const NormalData = NormalDataResult.value();
            Vertex.Normal                   = { NormalData[0], NormalData[1], NormalData[2] };
        }

        if ( UVNode != nullptr )
        {
            const auto UVDataResult = ReadVector( UVNode, 2U, "UV" );
            LUMEN_EXPECT_VALUE( UVDataResult );

            const Float32 *const UVData = UVDataResult.value();
            Vertex.UV                   = { UVData[0], UVData[1] };
        }

        if ( TangentNode != nullptr )
        {
            const auto TangentDataResult = ReadVector( TangentNode, 3U, "Tangent" );
            LUMEN_EXPECT_VALUE( TangentDataResult );

            const Float32 *const TangentData = TangentDataResult.value();
            Vertex.Tangent                   = { TangentData[0], TangentData[1], TangentData[2] };
        }

        Vertices.push_back( Vertex );
    }

    return Vertices;
}

LumenEngine::TExpected<LumenEngine::TVector<LumenEngine::UInt32>, LumenEngine::FString>
LumenEngine::Compiler::FBinarySerializer::ExtractIndices ( const FDLSLNode *InIndicesListNode )
{
    LUMEN_EXPECT( InIndicesListNode != nullptr, "Indices node is null." );
    LUMEN_EXPECT( InIndicesListNode->Type == EDLSLNodeType::List, "Indices must be a list." );

    TVector<UInt32> Indices;
    Indices.reserve( InIndicesListNode->ListValue.Count );

    for ( const FDLSLNode *IndexNode = InIndicesListNode->ListValue.Head; IndexNode != nullptr; IndexNode = IndexNode->Next )
    {
        if ( IndexNode->Type != EDLSLNodeType::Number )
        {
            return MakeUnexpected( "Each index entry must be a number." );
        }

        const Float64 Value = IndexNode->NumberValue;
        if ( Value < 0.0 or std::floor( Value ) != Value or Value > static_cast<Float64>( std::numeric_limits<UInt32>::max() ) )
        {
            return MakeUnexpected( "Index values must be non-negative UInt32 integers." );
        }

        Indices.push_back( static_cast<UInt32>( Value ) );
    }

    return Indices;
}

void LumenEngine::Compiler::FBinarySerializer::ExtractMeshConfig ( const FDLSLNode *InConfigNode, FLumenBinaryMeshHeader &OutHeader ) noexcept
{
    if ( InConfigNode == nullptr or InConfigNode->Type != EDLSLNodeType::Object )
    {
        return;
    }

    const FDLSLNode *const TopologyNode     = FindProperty( InConfigNode, "Topology" );
    const FDLSLNode *const CullModeNode     = FindProperty( InConfigNode, "CullMode" );
    const FDLSLNode *const WindingOrderNode = FindProperty( InConfigNode, "WindingOrder" );

    if ( TopologyNode != nullptr and TopologyNode->Type == EDLSLNodeType::Identifier )
    {
        const FStringView Topology = TopologyNode->GetString();
        if ( Topology == "PointList" )
        {
            OutHeader.Topology = 1U;
        }
        else if ( Topology == "LineList" )
        {
            OutHeader.Topology = 2U;
        }
        else if ( Topology == "TriangleList" )
        {
            OutHeader.Topology = 3U;
        }
    }

    if ( CullModeNode != nullptr and CullModeNode->Type == EDLSLNodeType::Identifier )
    {
        const FStringView CullMode = CullModeNode->GetString();
        if ( CullMode == "None" )
        {
            OutHeader.CullMode = 0U;
        }
        else if ( CullMode == "Front" )
        {
            OutHeader.CullMode = 1U;
        }
        else if ( CullMode == "Back" )
        {
            OutHeader.CullMode = 2U;
        }
        else if ( CullMode == "FrontAndBack" )
        {
            OutHeader.CullMode = 3U;
        }
    }

    if ( WindingOrderNode != nullptr and WindingOrderNode->Type == EDLSLNodeType::Identifier )
    {
        const FStringView WindingOrder = WindingOrderNode->GetString();
        if ( WindingOrder == "CW" )
        {
            OutHeader.WindingOrder = 0U;
        }
        else if ( WindingOrder == "CCW" )
        {
            OutHeader.WindingOrder = 1U;
        }
    }
}

void LumenEngine::Compiler::FBinarySerializer::ExtractRenderState ( const FDLSLNode *InStateNode, FLumenBinaryMaterialHeader &OutHeader ) noexcept
{
    if ( InStateNode == nullptr or InStateNode->Type != EDLSLNodeType::Object )
    {
        return;
    }

    OutHeader.PropertyCount = static_cast<UInt32>( InStateNode->ObjectValue.Count );

    const FDLSLNode *const BlendModeNode  = FindProperty( InStateNode, "BlendMode" );
    const FDLSLNode *const DepthTestNode  = FindProperty( InStateNode, "DepthTest" );
    const FDLSLNode *const DepthWriteNode = FindProperty( InStateNode, "DepthWrite" );
    const FDLSLNode *const CullModeNode   = FindProperty( InStateNode, "CullMode" );
    const FDLSLNode *const WireFrameNode  = FindProperty( InStateNode, "WireFrame" );

    if ( BlendModeNode != nullptr and BlendModeNode->Type == EDLSLNodeType::Identifier )
    {
        const FStringView BlendMode = BlendModeNode->GetString();
        if ( BlendMode == "Opaque" )
        {
            OutHeader.BlendMode = 0U;
        }
        else if ( BlendMode == "Alpha" )
        {
            OutHeader.BlendMode = 1U;
        }
        else if ( BlendMode == "Additive" )
        {
            OutHeader.BlendMode = 2U;
        }
    }

    if ( DepthTestNode != nullptr and DepthTestNode->Type == EDLSLNodeType::Identifier )
    {
        const FStringView DepthTest = DepthTestNode->GetString();
        if ( DepthTest == "Never" )
        {
            OutHeader.DepthTest = 0U;
        }
        else if ( DepthTest == "Less" )
        {
            OutHeader.DepthTest = 1U;
        }
        else if ( DepthTest == "Always" )
        {
            OutHeader.DepthTest = 2U;
        }
    }

    if ( DepthWriteNode != nullptr and DepthWriteNode->Type == EDLSLNodeType::Boolean )
    {
        OutHeader.DepthWrite = DepthWriteNode->BooleanValue ? 1U : 0U;
    }

    if ( CullModeNode != nullptr and CullModeNode->Type == EDLSLNodeType::Identifier )
    {
        const FStringView CullMode = CullModeNode->GetString();
        if ( CullMode == "None" )
        {
            OutHeader.CullMode = 0U;
        }
        else if ( CullMode == "Front" )
        {
            OutHeader.CullMode = 1U;
        }
        else if ( CullMode == "Back" )
        {
            OutHeader.CullMode = 2U;
        }
        else if ( CullMode == "FrontAndBack" )
        {
            OutHeader.CullMode = 3U;
        }
    }

    if ( WireFrameNode != nullptr and WireFrameNode->Type == EDLSLNodeType::Boolean )
    {
        OutHeader.WireFrame = WireFrameNode->BooleanValue ? 1U : 0U;
    }
}
