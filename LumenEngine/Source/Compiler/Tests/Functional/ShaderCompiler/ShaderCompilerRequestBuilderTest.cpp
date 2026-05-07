/**
 * @file ShaderCompilerRequestBuilderTest.cpp
 * @brief Tests for FShaderCompileRequestBuilder.
 */

#include "ShaderCompiler/ShaderCompilerRequest.hpp"
#include <gtest/gtest.h>

TEST( FShaderCompileRequestBuilder, BuildRequest )
{
    LumenEngine::Compiler::FShaderCompileRequest Request = LumenEngine::Compiler::FShaderCompileRequestBuilder()
                                                               .Path( "Shaders/Test.vert" )
                                                               .EntryPoint( "vmain" )
                                                               .Vertex()
                                                               .Optimize( LumenEngine::Compiler::EShaderOptimizationLevel::Size )
                                                               .Debug( true )
                                                               .Version( 450 )
                                                               .Macro( "DEBUG", "1" )
                                                               .Include( "Shaders/Include" )
                                                               .Build();

    EXPECT_EQ( Request.SourcePath, "Shaders/Test.vert" );
    EXPECT_EQ( Request.EntryPoint, "vmain" );
    EXPECT_EQ( Request.Stage, LumenEngine::Compiler::EShaderStage::Vertex );
    EXPECT_EQ( Request.Optimization, LumenEngine::Compiler::EShaderOptimizationLevel::Size );
    EXPECT_EQ( Request.bEmitDebugInfo, true );
    EXPECT_EQ( Request.GlslVersion, 450 );

    ASSERT_EQ( Request.Macros.size(), 1U );
    EXPECT_EQ( Request.Macros[0].Name, "DEBUG" );
    EXPECT_EQ( Request.Macros[0].Value, "1" );

    ASSERT_EQ( Request.IncludeDirectories.size(), 1U );
    EXPECT_EQ( Request.IncludeDirectories[0], "Shaders/Include" );
}
