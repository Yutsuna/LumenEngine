/**
 * @file ShaderCompilerIntegrationTest.cpp
 * @brief Integration tests for FShaderCompiler.
 */

#include "../../Functional/ShaderCompiler/ShaderCompilerTest.hpp"
#include "ShaderCompiler/ShaderCompiler.hpp"
#include "ShaderCompiler/ShaderCompilerRequest.hpp"

#include "Filesystem/Directory.hpp"
#include "Filesystem/File.hpp"
#include "Filesystem/FileSystem.hpp"
#include "Filesystem/Path.hpp"

#include <gtest/gtest.h>

namespace
{

class FShaderCompilerFixture : public ::testing::Test
{
protected:

    void SetUp () override
    {
        const testing::TestInfo *Info = ::testing::UnitTest::GetInstance()->current_test_info();

        LumenEngine::Compiler::FShaderCompilerConfig Config;
        Config.CacheDirectory = LumenEngine::FFileSystem::GetTempDirectory() / "ShaderCompilerTests" / Info->test_suite_name() / Info->name();

        LumenEngine::Filesystem::FDirectory::Delete( Config.CacheDirectory, true );
        LumenEngine::Filesystem::FDirectory::CreateDirectories( Config.CacheDirectory );

        Compiler = LumenEngine::MakeUnique<LumenEngine::Compiler::FShaderCompiler>( std::move( Config ) );
    }

    void TearDown () override
    {
        LumenEngine::Filesystem::FDirectory::Delete( Compiler->GetConfig().CacheDirectory, true );
    }

    LumenEngine::TUniquePtr<LumenEngine::Compiler::FShaderCompiler> Compiler;
};

} // namespace

TEST_F( FShaderCompilerFixture, CompileVertexShaderFromSource )
{
    LumenEngine::Compiler::FShaderCompileRequest Request = LumenEngine::Compiler::FShaderCompileRequestBuilder().Vertex().Build();

    LumenEngine::Compiler::FShaderCompileResult Result = Compiler->CompileShaderFromSource( LumenEngine::Compiler::GMinimalVertexShader, Request );

    ASSERT_TRUE( Result.IsSuccess() ) << Result.ErrorLog;
    EXPECT_TRUE( Result.Shader->IsValid() );
    EXPECT_EQ( Result.Shader->Stage, LumenEngine::Compiler::EShaderStage::Vertex );
    EXPECT_FALSE( Result.Shader->bFromCache );
}

TEST_F( FShaderCompilerFixture, CompileFragmentShaderFromSource )
{
    LumenEngine::Compiler::FShaderCompileRequest Request = LumenEngine::Compiler::FShaderCompileRequestBuilder().Fragment().Build();

    LumenEngine::Compiler::FShaderCompileResult Result = Compiler->CompileShaderFromSource( LumenEngine::Compiler::GMinimalFragmentShader, Request );

    ASSERT_TRUE( Result.IsSuccess() ) << Result.ErrorLog;
    EXPECT_EQ( Result.Shader->Stage, LumenEngine::Compiler::EShaderStage::Fragment );
}

TEST_F( FShaderCompilerFixture, CompileComputeShaderFromSource )
{
    LumenEngine::Compiler::FShaderCompileRequest Request = LumenEngine::Compiler::FShaderCompileRequestBuilder().Compute().Build();

    LumenEngine::Compiler::FShaderCompileResult Result = Compiler->CompileShaderFromSource( LumenEngine::Compiler::GComputeShader, Request );

    ASSERT_TRUE( Result.IsSuccess() ) << Result.ErrorLog;
    EXPECT_EQ( Result.Shader->Stage, LumenEngine::Compiler::EShaderStage::Compute );
}

TEST_F( FShaderCompilerFixture, CacheHit )
{
    LumenEngine::Compiler::FShaderCompileRequest Request = LumenEngine::Compiler::FShaderCompileRequestBuilder().Vertex().Build();

    // First compile
    LumenEngine::Compiler::FShaderCompileResult Result1 = Compiler->CompileShaderFromSource( LumenEngine::Compiler::GMinimalVertexShader, Request );
    ASSERT_TRUE( Result1.IsSuccess() );
    EXPECT_FALSE( Result1.Shader->bFromCache );

    // Second compile
    LumenEngine::Compiler::FShaderCompileResult Result2 = Compiler->CompileShaderFromSource( LumenEngine::Compiler::GMinimalVertexShader, Request );
    ASSERT_TRUE( Result2.IsSuccess() );
    EXPECT_TRUE( Result2.Shader->bFromCache );
    EXPECT_EQ( Result1.Shader->Hash, Result2.Shader->Hash );
}

TEST_F( FShaderCompilerFixture, MacroSupport )
{
    LumenEngine::Compiler::FShaderCompileRequest RequestWithMacro = LumenEngine::Compiler::FShaderCompileRequestBuilder().Fragment().Macro( "TEST_MACRO" ).Build();

    LumenEngine::Compiler::FShaderCompileRequest RequestWithoutMacro = LumenEngine::Compiler::FShaderCompileRequestBuilder().Fragment().Build();

    LumenEngine::Compiler::FShaderCompileResult Result1 = Compiler->CompileShaderFromSource( LumenEngine::Compiler::GShaderWithMacro, RequestWithMacro );
    LumenEngine::Compiler::FShaderCompileResult Result2 = Compiler->CompileShaderFromSource( LumenEngine::Compiler::GShaderWithMacro, RequestWithoutMacro );

    ASSERT_TRUE( Result1.IsSuccess() );
    ASSERT_TRUE( Result2.IsSuccess() );
    EXPECT_NE( Result1.Shader->Hash, Result2.Shader->Hash );
}

TEST_F( FShaderCompilerFixture, ClearCache )
{
    LumenEngine::Compiler::FShaderCompileRequest Request = LumenEngine::Compiler::FShaderCompileRequestBuilder().Vertex().Build();

    EXPECT_TRUE( Compiler->CompileShaderFromSource( LumenEngine::Compiler::GMinimalVertexShader, Request ).IsSuccess() );
    EXPECT_GT( Compiler->ClearCache(), 0U );

    LumenEngine::Compiler::FShaderCompileResult Result = Compiler->CompileShaderFromSource( LumenEngine::Compiler::GMinimalVertexShader, Request );
    EXPECT_FALSE( Result.Shader->bFromCache );
}
