/**
 * @file LumenDLSLParserTest.cpp
 * @brief Tests for the DLSL parser.
 */

#include "LumenCompilerTest.hpp"

namespace LumenEngine
{

TEST_F( FParserFixture, ParsesMinimalMeshDocument )
{
    Compiler::FDLSLDocument *Doc = MustParse( GMinimalMeshDLSL );

    ASSERT_NE( Doc, nullptr );
    ASSERT_NE( Doc->FirstBlock, nullptr );
    EXPECT_EQ( Doc->FirstBlock->BlockType, "Mesh" );
    EXPECT_EQ( Doc->FirstBlock->Name, "Cube" );
    EXPECT_EQ( Doc->FirstBlock->Next, nullptr );
}

TEST_F( FParserFixture, ParsesMinimalMaterialDocument )
{
    Compiler::FDLSLDocument *Doc = MustParse( GMinimalMaterialDLSL );

    ASSERT_NE( Doc, nullptr );
    ASSERT_NE( Doc->FirstBlock, nullptr );
    EXPECT_EQ( Doc->FirstBlock->BlockType, "Material" );
    EXPECT_EQ( Doc->FirstBlock->Name, "PBR" );
}

TEST_F( FParserFixture, ParsesMultiBlockDocument )
{
    Compiler::FDLSLDocument *Doc = MustParse( GMultiBlockDLSL );
    ASSERT_NE( Doc, nullptr );

    const Compiler::FDLSLRootBlock *Block = Doc->FirstBlock;
    ASSERT_NE( Block, nullptr );
    EXPECT_EQ( Block->BlockType, "Mesh" );
    EXPECT_EQ( Block->Name, "Hero" );

    Block = Block->Next;
    ASSERT_NE( Block, nullptr );
    EXPECT_EQ( Block->BlockType, "Mesh" );
    EXPECT_EQ( Block->Name, "Prop" );

    Block = Block->Next;
    ASSERT_NE( Block, nullptr );
    EXPECT_EQ( Block->BlockType, "Material" );
    EXPECT_EQ( Block->Name, "Ground" );

    EXPECT_EQ( Block->Next, nullptr );
}

TEST_F( FParserFixture, ParsesVectorNode )
{
    constexpr const AnsiChar *Source =
        "@Mesh \"V\" { Vertices: [ { Position: (1.5, 2.5, 3.5) } ] Indices: [ 0 ] @Config { Topology: TriangleList CullMode: Back WindingOrder: CCW } }";
    Compiler::FDLSLDocument *Doc = MustParse( Source );
    ASSERT_NE( Doc, nullptr );
    ASSERT_NE( Doc->FirstBlock, nullptr );
    ASSERT_NE( Doc->FirstBlock->Body, nullptr );
}

TEST_F( FParserFixture, ParsesBooleanNode )
{
    constexpr const AnsiChar *Source = "@Material \"M\" { @RenderState { DepthWrite: True WireFrame: False BlendMode: Opaque DepthTest: Less CullMode: Back } }";
    Compiler::FDLSLDocument *Doc     = MustParse( Source );
    ASSERT_NE( Doc, nullptr );
    ASSERT_NE( Doc->FirstBlock, nullptr );
}

TEST_F( FParserFixture, EmptyDocumentIsValid )
{
    Compiler::FDLSLParser Parser( "", *Allocator );
    TExpected<Compiler::FDLSLDocument * /**/, FString> Result = Parser.Parse();

    ASSERT_TRUE( Result.has_value() );
    EXPECT_EQ( Result.value()->FirstBlock, nullptr );
}

TEST_F( FParserFixture, MissingClosingBraceReturnsError )
{
    Compiler::FDLSLParser Parser( "@Mesh \"X\" {", *Allocator );
    TExpected<Compiler::FDLSLDocument * /**/, FString> Result = Parser.Parse();

    EXPECT_FALSE( Result.has_value() );
    EXPECT_FALSE( Result.error().empty() );
}

TEST_F( FParserFixture, OutOfMemoryReturnsError )
{
    alignas( 16 ) Byte TinyBuf[1];
    HAL::FLinearAllocator TinyAllocator( TinyBuf, sizeof( TinyBuf ) );

    Compiler::FDLSLParser Parser( GMinimalMeshDLSL, TinyAllocator );
    TExpected<Compiler::FDLSLDocument * /**/, FString> Result = Parser.Parse();

    EXPECT_FALSE( Result.has_value() );
}

} // namespace LumenEngine
