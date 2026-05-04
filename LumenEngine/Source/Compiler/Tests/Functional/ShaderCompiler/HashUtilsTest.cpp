/**
 * @file HashUtilsTest.cpp
 * @brief Tests for Internal::FHashUtils.
 */

#include "ShaderCompiler/Internal/HashUtils.hpp"
#include "ShaderCompiler/ShaderCompilerRequest.hpp"
#include <gtest/gtest.h>

TEST( FHashUtils, Deterministic )
{
    LumenEngine::Compiler::FShaderCompileRequest Request;
    Request.Stage      = LumenEngine::Compiler::EShaderStage::Vertex;
    Request.EntryPoint = "main";

    const LumenEngine::FString View = "#version 450\nvoid main() {}";

    const LumenEngine::Compiler::FSourceHash H1 = LumenEngine::Compiler::Internal::FHashUtils::ComputeRequestHash( View, Request );
    const LumenEngine::Compiler::FSourceHash H2 = LumenEngine::Compiler::Internal::FHashUtils::ComputeRequestHash( View, Request );

    EXPECT_EQ( H1, H2 );
}

TEST( FHashUtils, DiffersBySource )
{
    LumenEngine::Compiler::FShaderCompileRequest Request;
    Request.Stage = LumenEngine::Compiler::EShaderStage::Vertex;

    const LumenEngine::Compiler::FSourceHash H1 = LumenEngine::Compiler::Internal::FHashUtils::ComputeRequestHash( "Source1", Request );
    const LumenEngine::Compiler::FSourceHash H2 = LumenEngine::Compiler::Internal::FHashUtils::ComputeRequestHash( "Source2", Request );

    EXPECT_NE( H1, H2 );
}

TEST( FHashUtils, DiffersByStage )
{
    const LumenEngine::FString Source = "Source";
    LumenEngine::Compiler::FShaderCompileRequest R1;
    R1.Stage = LumenEngine::Compiler::EShaderStage::Vertex;
    LumenEngine::Compiler::FShaderCompileRequest R2;
    R2.Stage = LumenEngine::Compiler::EShaderStage::Fragment;

    const LumenEngine::Compiler::FSourceHash H1 = LumenEngine::Compiler::Internal::FHashUtils::ComputeRequestHash( Source, R1 );
    const LumenEngine::Compiler::FSourceHash H2 = LumenEngine::Compiler::Internal::FHashUtils::ComputeRequestHash( Source, R2 );

    EXPECT_NE( H1, H2 );
}

TEST( FHashUtils, DiffersByMacro )
{
    const LumenEngine::FString Source = "Source";
    LumenEngine::Compiler::FShaderCompileRequest R1;
    LumenEngine::Compiler::FShaderCompileRequest R2;
    R2.Macros.push_back( { .Name = "DEBUG", .Value = "1" } );

    const LumenEngine::Compiler::FSourceHash H1 = LumenEngine::Compiler::Internal::FHashUtils::ComputeRequestHash( Source, R1 );
    const LumenEngine::Compiler::FSourceHash H2 = LumenEngine::Compiler::Internal::FHashUtils::ComputeRequestHash( Source, R2 );

    EXPECT_NE( H1, H2 );
}
