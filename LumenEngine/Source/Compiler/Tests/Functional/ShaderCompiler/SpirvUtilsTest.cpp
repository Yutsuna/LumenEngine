/**
 * @file SpirvUtilsTest.cpp
 * @brief Tests for Internal::FSpirvUtils.
 */

#include "ShaderCompiler/Internal/SpirvUtils.hpp"
#include "ShaderCompiler/Internal/GlslangBackend.hpp"
#include "ShaderCompiler/ShaderCompilerRequest.hpp"

#include <gtest/gtest.h>

namespace
{

class FSpirvUtilsTest : public ::testing::Test
{
protected:

    void SetUp () override
    {
        if ( not LumenEngine::Compiler::Internal::FGlslangBackend::Initialize() )
        {
            return;
        }
    }

    void TearDown () override
    {
        LumenEngine::Compiler::Internal::FGlslangBackend::Finalize();
    }
};

} // namespace

TEST_F( FSpirvUtilsTest, ValidateAndDisassemble )
{
    LumenEngine::Compiler::FShaderCompileRequest Request;
    Request.Stage      = LumenEngine::Compiler::EShaderStage::Vertex;
    Request.EntryPoint = "main";

    LumenEngine::FStringView Source = "#version 450\nvoid main() { gl_Position = vec4(1.0); }";
    LumenEngine::Compiler::FSpirVBlob SpirV;
    LumenEngine::FString Error;

    ASSERT_TRUE( LumenEngine::Compiler::Internal::FGlslangBackend::Compile( Source, Request, SpirV, Error ) ) << Error;

    LumenEngine::FString Validation = LumenEngine::Compiler::Internal::FSpirvUtils::Validate( SpirV );
    EXPECT_EQ( Validation, "Validation successful." );

    LumenEngine::FString Assembly = LumenEngine::Compiler::Internal::FSpirvUtils::Disassemble( SpirV );
    EXPECT_FALSE( Assembly.empty() );
    EXPECT_NE( Assembly.find( "OpEntryPoint" ), LumenEngine::FString::npos );
}

TEST_F( FSpirvUtilsTest, ValidateEmptyBlob )
{
    LumenEngine::Compiler::FSpirVBlob Empty;
    LumenEngine::FString Validation = LumenEngine::Compiler::Internal::FSpirvUtils::Validate( Empty );
    EXPECT_NE( Validation, "Validation successful." );
}
