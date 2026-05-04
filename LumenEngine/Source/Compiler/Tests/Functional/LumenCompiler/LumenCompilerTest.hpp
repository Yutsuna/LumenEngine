/**
 * @file LumenCompilerTest.hpp
 * @brief LumenCompilerTest class definition.
 */

#pragma once

#include "LumenCompiler/Internal/DLSLParser.hpp"
#include "LumenCompiler/Internal/DLSLTypes.hpp"

#include "Container/UniquePtr.hpp"

#include <gtest/gtest.h>

namespace LumenEngine
{

static constexpr USize GScratchSize = 512ULL * 1024ULL;

/**
 * @class FParserFixture
 * @brief Test fixture for FDLSLParser tests
 */
class FParserFixture : public ::testing::Test
{

protected:

    void SetUp () override
    {
        ScratchBuffer = MakeUnique<Byte[]>( GScratchSize );
        Allocator     = MakeUnique<HAL::FLinearAllocator>( ScratchBuffer.Get(), GScratchSize );
    }

    Compiler::FDLSLDocument *MustParse ( const FStringView InSource )
    {
        Compiler::FDLSLParser Parser( InSource, *Allocator );
        TExpected<Compiler::FDLSLDocument *, FString> Result = Parser.Parse();

        EXPECT_TRUE( Result.has_value() ) << "Parse failed: " << ( Result.has_value() ? "" : Result.error() );
        return Result.value_or( nullptr );
    }

protected:

    TUniquePtr<Byte[]> ScratchBuffer;
    TUniquePtr<HAL::FLinearAllocator> Allocator;
};

static constexpr const AnsiChar *GMinimalMeshDLSL = R"dlsl(
@Mesh "Cube"
{
    Vertices: [
        { Position: (0.0, 0.0, 0.0), Normal: (0.0, 1.0, 0.0), UV: (0.0, 0.0), Tangent: (1.0, 0.0, 0.0) },
        { Position: (1.0, 0.0, 0.0), Normal: (0.0, 1.0, 0.0), UV: (1.0, 0.0), Tangent: (1.0, 0.0, 0.0) },
        { Position: (0.0, 1.0, 0.0), Normal: (0.0, 1.0, 0.0), UV: (0.0, 1.0), Tangent: (1.0, 0.0, 0.0) },
    ]
    Indices: [ 0, 1, 2 ]
    @Config
    {
        Topology:     TriangleList
        CullMode:     Back
        WindingOrder: CCW
    }
}
)dlsl";

static constexpr const AnsiChar *GMinimalMaterialDLSL = R"dlsl(
@Material "PBR"
{
    @RenderState
    {
        BlendMode:  Opaque
        DepthTest:  Less
        DepthWrite: True
        CullMode:   Back
        WireFrame:  False
    }
}
)dlsl";

static constexpr const AnsiChar *GMultiBlockDLSL = R"dlsl(
@Mesh "Hero"
{
    Vertices: [
        { Position: (0.0, 0.0, 0.0), Normal: (0.0, 1.0, 0.0), UV: (0.0, 0.0), Tangent: (1.0, 0.0, 0.0) },
    ]
    Indices: [ 0 ]
    @Config { Topology: TriangleList, CullMode: Back, WindingOrder: CCW }
}
@Mesh "Prop"
{
    Vertices: [
        { Position: (1.0, 1.0, 1.0), Normal: (0.0, 0.0, 1.0), UV: (1.0, 1.0), Tangent: (0.0, 1.0, 0.0) },
    ]
    Indices: [ 0 ]
    @Config { Topology: TriangleList, CullMode: Back, WindingOrder: CCW }
}
@Material "Ground"
{
    @RenderState { BlendMode: Opaque, DepthTest: Less, DepthWrite: True, CullMode: Back, WireFrame: False }
}
)dlsl";

} // namespace LumenEngine
