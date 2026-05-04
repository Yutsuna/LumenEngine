/**
 * @file LumenCompilerIntegrationTest.cpp
 * @brief Integration tests for the FLumenCompiler class
 */

#include "../../Functional/LumenCompiler/LumenCompilerTest.hpp"
#include "LumenCompiler/LumenCompiler.hpp"
#include "LumenCompiler/LumenCompilerTypes.hpp"

#include <gtest/gtest.h>

namespace LumenEngine
{

namespace Compiler
{

    namespace
    {

        class FLumenCompilerFixture : public ::testing::Test
        {
        protected:

            void SetUp () override
            {
                const testing::TestInfo *Info = ::testing::UnitTest::GetInstance()->current_test_info();

                FLumenCompilerConfig Config;
                Config.CacheDirectory    = std::filesystem::temp_directory_path() / "LumenCompilerTests" / Info->test_suite_name() / Info->name();
                Config.ScratchBufferSize = 512ULL * 1024ULL;
                Config.bVerboseLogging   = false;

                Compiler = MakeUnique<FLumenCompiler>( std::move( Config ) );

                std::error_code Ec;
                std::filesystem::remove_all( Compiler->GetConfig().CacheDirectory, Ec );
                std::filesystem::create_directories( Compiler->GetConfig().CacheDirectory, Ec );
            }

            void TearDown () override
            {
                std::error_code Ec;
                std::filesystem::remove_all( Compiler->GetConfig().CacheDirectory, Ec );
            }

            TUniquePtr<FLumenCompiler> Compiler;
        };

    } // namespace

    TEST_F( FLumenCompilerFixture, CompileMeshFromSourceSucceeds )
    {
        FLumenCompileRequest Request;
        Request.TargetBlockName   = "Cube";
        Request.ExpectedBlockType = "Mesh";

        const FLumenCompileResult Result = Compiler->CompileAssetFromSource( GMinimalMeshDLSL, Request );

        ASSERT_TRUE( Result.IsSuccess() ) << Result.ErrorLog;
        EXPECT_GT( Result.Asset->BinaryBlob.size(), 0U );
        EXPECT_EQ( Result.Asset->AssetType, EAssetType::Mesh );
        EXPECT_EQ( Result.Asset->BlockName, "Cube" );
        EXPECT_FALSE( Result.Asset->bFromCache );
    }

    TEST_F( FLumenCompilerFixture, CompileMaterialFromSourceSucceeds )
    {
        FLumenCompileRequest Request;
        Request.TargetBlockName   = "PBR";
        Request.ExpectedBlockType = "Material";

        const FLumenCompileResult Result = Compiler->CompileAssetFromSource( GMinimalMaterialDLSL, Request );

        ASSERT_TRUE( Result.IsSuccess() ) << Result.ErrorLog;
        EXPECT_EQ( Result.Asset->AssetType, EAssetType::Material );
    }

    TEST_F( FLumenCompilerFixture, SecondCompileProducesCacheHit )
    {
        FLumenCompileRequest Request;
        Request.TargetBlockName   = "Cube";
        Request.ExpectedBlockType = "Mesh";

        const FLumenCompileResult FirstResult = Compiler->CompileAssetFromSource( GMinimalMeshDLSL, Request );
        ASSERT_TRUE( FirstResult.IsSuccess() ) << FirstResult.ErrorLog;
        EXPECT_EQ( Compiler->GetCacheHitCount(), 0ULL );

        const FLumenCompileResult SecondResult = Compiler->CompileAssetFromSource( GMinimalMeshDLSL, Request );
        ASSERT_TRUE( SecondResult.IsSuccess() ) << SecondResult.ErrorLog;
        EXPECT_GE( Compiler->GetCacheHitCount(), 1ULL );

        ASSERT_EQ( FirstResult.Asset->BinaryBlob.size(), SecondResult.Asset->BinaryBlob.size() );
        EXPECT_EQ( std::memcmp( FirstResult.Asset->BinaryBlob.data(), SecondResult.Asset->BinaryBlob.data(), FirstResult.Asset->BinaryBlob.size() ), 0 );
    }

    TEST_F( FLumenCompilerFixture, DifferentSourceProducesNoCacheHit )
    {
        FLumenCompileRequest Request;
        Request.TargetBlockName   = "Cube";
        Request.ExpectedBlockType = "Mesh";

        EXPECT_TRUE( Compiler->CompileAssetFromSource( GMinimalMeshDLSL, Request ).IsSuccess() );

        FString Modified( GMinimalMeshDLSL );
        const USize Pos = Modified.find( "(0.0, 0.0, 0.0)" );
        if ( Pos != FString::npos )
        {
            Modified.replace( Pos, 15, "(9.9, 9.9, 9.9)" );
        }

        const UInt64 HitsBefore = Compiler->GetCacheHitCount();
        EXPECT_TRUE( Compiler->CompileAssetFromSource( Modified, Request ).IsSuccess() );
        EXPECT_EQ( Compiler->GetCacheHitCount(), HitsBefore );
    }

    TEST_F( FLumenCompilerFixture, CompileByNameFromMultiBlockDocument )
    {
        FLumenCompileRequest Request;
        Request.TargetBlockName   = "Prop";
        Request.ExpectedBlockType = "Mesh";

        const FLumenCompileResult Result = Compiler->CompileAssetFromSource( GMultiBlockDLSL, Request );
        ASSERT_TRUE( Result.IsSuccess() ) << Result.ErrorLog;
        EXPECT_EQ( Result.Asset->BlockName, "Prop" );
        EXPECT_EQ( Result.Asset->AssetType, EAssetType::Mesh );
    }

    TEST_F( FLumenCompilerFixture, CompileMaterialFromMultiBlockDocument )
    {
        FLumenCompileRequest Request;
        Request.TargetBlockName   = "Ground";
        Request.ExpectedBlockType = "Material";

        const FLumenCompileResult Result = Compiler->CompileAssetFromSource( GMultiBlockDLSL, Request );
        ASSERT_TRUE( Result.IsSuccess() ) << Result.ErrorLog;
        EXPECT_EQ( Result.Asset->AssetType, EAssetType::Material );
    }

    TEST_F( FLumenCompilerFixture, CompileFirstBlockWhenNoNameSpecified )
    {
        FLumenCompileRequest Request;
        const FLumenCompileResult Result = Compiler->CompileAssetFromSource( GMultiBlockDLSL, Request );

        ASSERT_TRUE( Result.IsSuccess() ) << Result.ErrorLog;
        EXPECT_EQ( Result.Asset->AssetType, EAssetType::Mesh );
    }

    TEST_F( FLumenCompilerFixture, EmptySourceReturnsError )
    {
        FLumenCompileRequest Request;

        const FLumenCompileResult Result = Compiler->CompileAssetFromSource( "", Request );
        EXPECT_FALSE( Result.IsSuccess() );
        EXPECT_NE( Result.Error, ELumenCompilerError::None );
    }

    TEST_F( FLumenCompilerFixture, BlockNotFoundReturnsError )
    {
        FLumenCompileRequest Request;
        Request.TargetBlockName   = "NonExistentBlock";
        Request.ExpectedBlockType = "Mesh";

        const FLumenCompileResult Result = Compiler->CompileAssetFromSource( GMinimalMeshDLSL, Request );
        EXPECT_FALSE( Result.IsSuccess() );
        EXPECT_FALSE( Result.ErrorLog.empty() );
    }

    TEST_F( FLumenCompilerFixture, BlockTypeMismatchReturnsError )
    {
        FLumenCompileRequest Request;
        Request.TargetBlockName   = "Cube";
        Request.ExpectedBlockType = "Material";

        const FLumenCompileResult Result = Compiler->CompileAssetFromSource( GMinimalMeshDLSL, Request );
        EXPECT_FALSE( Result.IsSuccess() );
        EXPECT_FALSE( Result.ErrorLog.empty() );
    }

    TEST_F( FLumenCompilerFixture, MalformedDLSLReturnsParseError )
    {
        constexpr const AnsiChar *Malformed = "@Mesh \"Broken\" { Vertices: [";

        FLumenCompileRequest Request;
        const FLumenCompileResult Result = Compiler->CompileAssetFromSource( Malformed, Request );
        EXPECT_FALSE( Result.IsSuccess() );
        EXPECT_FALSE( Result.ErrorLog.empty() );
    }

    TEST_F( FLumenCompilerFixture, CompileResultIsDeterministic )
    {
        FLumenCompileRequest Request;
        Request.TargetBlockName   = "Cube";
        Request.ExpectedBlockType = "Mesh";

        Compiler->ClearCache();

        const FLumenCompileResult R1 = Compiler->CompileAssetFromSource( GMinimalMeshDLSL, Request );
        ASSERT_TRUE( R1.IsSuccess() );

        Compiler->ClearCache();

        const FLumenCompileResult R2 = Compiler->CompileAssetFromSource( GMinimalMeshDLSL, Request );
        ASSERT_TRUE( R2.IsSuccess() );

        ASSERT_EQ( R1.Asset->BinaryBlob.size(), R2.Asset->BinaryBlob.size() );
        EXPECT_EQ( std::memcmp( R1.Asset->BinaryBlob.data(), R2.Asset->BinaryBlob.data(), R1.Asset->BinaryBlob.size() ), 0 );
    }

    TEST_F( FLumenCompilerFixture, ClearCacheResetsCounters )
    {
        FLumenCompileRequest Request;
        Request.TargetBlockName   = "Cube";
        Request.ExpectedBlockType = "Mesh";

        EXPECT_TRUE( Compiler->CompileAssetFromSource( GMinimalMeshDLSL, Request ).IsSuccess() );
        EXPECT_TRUE( Compiler->CompileAssetFromSource( GMinimalMeshDLSL, Request ).IsSuccess() );

        EXPECT_GE( Compiler->GetCacheHitCount(), 1ULL );
        Compiler->ClearCache();

        const UInt64 HitsBefore = Compiler->GetCacheHitCount();
        EXPECT_TRUE( Compiler->CompileAssetFromSource( GMinimalMeshDLSL, Request ).IsSuccess() );
        EXPECT_EQ( Compiler->GetCacheHitCount(), HitsBefore );
    }

    TEST_F( FLumenCompilerFixture, HashIsDeterministicForSameInput )
    {
        FLumenCompileRequest Request;
        Request.TargetBlockName = "Cube";

        const FAssetHash H1 = Compiler->ComputeHash( GMinimalMeshDLSL, Request );
        const FAssetHash H2 = Compiler->ComputeHash( GMinimalMeshDLSL, Request );

        EXPECT_EQ( H1, H2 );
    }

    TEST_F( FLumenCompilerFixture, HashDiffersForDifferentBlockNames )
    {
        FLumenCompileRequest RequestA;
        RequestA.TargetBlockName = "A";

        FLumenCompileRequest RequestB;
        RequestB.TargetBlockName = "B";

        const FAssetHash HA = Compiler->ComputeHash( GMinimalMeshDLSL, RequestA );
        const FAssetHash HB = Compiler->ComputeHash( GMinimalMeshDLSL, RequestB );

        EXPECT_NE( HA, HB );
    }

    TEST_F( FLumenCompilerFixture, HashDiffersForDifferentSources )
    {
        FLumenCompileRequest Request;
        Request.TargetBlockName = "Cube";

        const FAssetHash H1 = Compiler->ComputeHash( GMinimalMeshDLSL, Request );
        const FAssetHash H2 = Compiler->ComputeHash( GMinimalMaterialDLSL, Request );

        EXPECT_NE( H1, H2 );
    }

} // namespace Compiler

} // namespace LumenEngine
