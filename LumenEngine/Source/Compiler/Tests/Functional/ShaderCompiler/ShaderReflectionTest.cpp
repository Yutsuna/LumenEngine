/**
 * @file ShaderReflectionTest.cpp
 * @brief Tests for Internal::FSpirvReflector.
 */

#include "ShaderCompiler/Internal/GlslangBackend.hpp"
#include "ShaderCompiler/Internal/SpirvReflector.hpp"
#include "ShaderCompiler/ShaderCompilerRequest.hpp"
#include "ShaderCompilerTest.hpp"

#include <gtest/gtest.h>

namespace
{

class FShaderReflectionTest : public ::testing::Test
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

TEST_F( FShaderReflectionTest, ReflectVertexInputs )
{
    LumenEngine::Compiler::FShaderCompileRequest Request;
    Request.Stage      = LumenEngine::Compiler::EShaderStage::Vertex;
    Request.EntryPoint = "main";

    LumenEngine::Compiler::FSpirVBlob SpirV;
    LumenEngine::FString Error;
    ASSERT_TRUE( LumenEngine::Compiler::Internal::FGlslangBackend::Compile( LumenEngine::Compiler::GMinimalVertexShader, Request, SpirV, Error ) ) << Error;

    LumenEngine::Compiler::FShaderReflection Reflection;
    ASSERT_TRUE( LumenEngine::Compiler::Internal::FSpirvReflector::Reflect( SpirV, Request.Stage, Reflection, Error ) ) << Error;

    ASSERT_EQ( Reflection.VertexInputs.size(), 1U );
    EXPECT_EQ( Reflection.VertexInputs[0].Name, "inPosition" );
    EXPECT_EQ( Reflection.VertexInputs[0].Location, 0U );
}

TEST_F( FShaderReflectionTest, ReflectUniformBuffer )
{
    LumenEngine::Compiler::FShaderCompileRequest Request;
    Request.Stage      = LumenEngine::Compiler::EShaderStage::Fragment;
    Request.EntryPoint = "main";

    LumenEngine::Compiler::FSpirVBlob SpirV;
    LumenEngine::FString Error;
    ASSERT_TRUE( LumenEngine::Compiler::Internal::FGlslangBackend::Compile( LumenEngine::Compiler::GShaderWithUniform, Request, SpirV, Error ) ) << Error;

    LumenEngine::Compiler::FShaderReflection Reflection;
    ASSERT_TRUE( LumenEngine::Compiler::Internal::FSpirvReflector::Reflect( SpirV, Request.Stage, Reflection, Error ) ) << Error;

    ASSERT_EQ( Reflection.ResourceBindings.size(), 1U );
    EXPECT_EQ( Reflection.ResourceBindings[0].Name, "UBO" );
    EXPECT_EQ( Reflection.ResourceBindings[0].DescriptorType, "UniformBuffer" );
    EXPECT_EQ( Reflection.ResourceBindings[0].Set, 0U );
    EXPECT_EQ( Reflection.ResourceBindings[0].Binding, 0U );
}

TEST_F( FShaderReflectionTest, ReflectComputeLocalSize )
{
    LumenEngine::Compiler::FShaderCompileRequest Request;
    Request.Stage      = LumenEngine::Compiler::EShaderStage::Compute;
    Request.EntryPoint = "main";

    LumenEngine::Compiler::FSpirVBlob SpirV;
    LumenEngine::FString Error;
    ASSERT_TRUE( LumenEngine::Compiler::Internal::FGlslangBackend::Compile( LumenEngine::Compiler::GComputeShader, Request, SpirV, Error ) ) << Error;

    LumenEngine::Compiler::FShaderReflection Reflection;
    ASSERT_TRUE( LumenEngine::Compiler::Internal::FSpirvReflector::Reflect( SpirV, Request.Stage, Reflection, Error ) ) << Error;

    EXPECT_EQ( Reflection.LocalSizeX, 1U );
    EXPECT_EQ( Reflection.LocalSizeY, 1U );
    EXPECT_EQ( Reflection.LocalSizeZ, 1U );
}
