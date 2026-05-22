/**
 * @file AssetCompilerHotReloadStressTest.cpp
 * @brief Integration stress tests for FAssetCompiler hot reload under concurrent file writes.
 */

#include "../../../../Compiler/Tests/Functional/LumenCompiler/LumenCompilerTest.hpp"
#include "../../../../Compiler/Tests/Functional/ShaderCompiler/ShaderCompilerTest.hpp"

#include "Assets/AssetCompiler.hpp"

#include "Filesystem/Directory.hpp"
#include "Filesystem/File.hpp"
#include "Filesystem/FileSystem.hpp"
#include "Filesystem/Path.hpp"

#include <gtest/gtest.h>

#include <atomic>
#include <barrier>
#include <chrono>
#include <thread>

namespace
{

inline constexpr const LumenEngine::AnsiChar *GStressVertexShader = R"glsl(
#version 450
#define STRESS_VARIANT 1
layout(location = 0) in vec3 inPosition;
void main()
{
    gl_Position = vec4( inPosition, 1.0 );
}
)glsl";

inline constexpr const LumenEngine::AnsiChar *GStressFragmentShader = R"glsl(
#version 450
#define STRESS_VARIANT 1
layout(location = 0) out vec4 outColor;
void main()
{
    outColor = vec4( 1.0, 0.0, 0.0, 1.0 );
}
)glsl";

inline constexpr const LumenEngine::AnsiChar *GStressComputeShader = R"glsl(
#version 450
#define STRESS_VARIANT 1
layout( local_size_x = 1, local_size_y = 1, local_size_z = 1 ) in;
layout( set = 0, binding = 0 ) buffer Storage
{
    float data[];
} Buffer;
void main()
{
    Buffer.data[gl_GlobalInvocationID.x] *= 2.0;
}
)glsl";

struct FAssetFileSpec
{
    LumenEngine::Filesystem::FPath Path;
    const LumenEngine::AnsiChar *PrimaryContent;
    const LumenEngine::AnsiChar *SecondaryContent;
};

[[nodiscard]] inline LumenEngine::Bool WriteAssetFile ( const LumenEngine::Filesystem::FPath &InPath, const LumenEngine::AnsiChar *InContent ) noexcept
{
    return LumenEngine::Filesystem::FFile::WriteAllText( InPath, LumenEngine::FString( InContent ) ).has_value();
}

class FAssetCompilerHotReloadFixture : public ::testing::Test
{
protected:

    void SetUp () override
    {
        const testing::TestInfo *Info = ::testing::UnitTest::GetInstance()->current_test_info();

        auto CurrentDirectoryResult = LumenEngine::FFileSystem::GetCurrentDirectory();
        ASSERT_TRUE( CurrentDirectoryResult.has_value() );
        OriginalCurrentDirectory     = *CurrentDirectoryResult;
        bHasOriginalCurrentDirectory = true;

        TestRoot = LumenEngine::FFileSystem::GetTempDirectory() / "AssetCompilerHotReloadTests" / Info->test_suite_name() / Info->name();
        if ( LumenEngine::Filesystem::FDirectory::Exists( TestRoot ) )
        {
            ASSERT_TRUE( LumenEngine::Filesystem::FDirectory::Delete( TestRoot, true ).has_value() );
        }

        ASSERT_TRUE( LumenEngine::Filesystem::FDirectory::CreateDirectories( TestRoot ).has_value() );
        ASSERT_TRUE( LumenEngine::FFileSystem::SetCurrentDirectory( TestRoot ).has_value() );
        bChangedCurrentDirectory = true;

        AssetsRoot = TestRoot / "Assets";
        ASSERT_TRUE( LumenEngine::Filesystem::FDirectory::CreateDirectories( AssetsRoot / "Materials" ).has_value() );
        ASSERT_TRUE( LumenEngine::Filesystem::FDirectory::CreateDirectories( AssetsRoot / "Meshes" ).has_value() );
        ASSERT_TRUE( LumenEngine::Filesystem::FDirectory::CreateDirectories( AssetsRoot / "Shaders" ).has_value() );

        const LumenEngine::TVector<FAssetFileSpec> AssetFiles = {
            { AssetsRoot / "Materials" / "PrimaryMaterial.lumen", LumenEngine::GMinimalMaterialDLSL, LumenEngine::GMultiBlockDLSL },
            { AssetsRoot / "Materials" / "SecondaryMaterial.lumen", LumenEngine::GMultiBlockDLSL, LumenEngine::GMinimalMaterialDLSL },
            { AssetsRoot / "Meshes" / "PrimaryMesh.lumen", LumenEngine::GMinimalMeshDLSL, LumenEngine::GMultiBlockDLSL },
            { AssetsRoot / "Meshes" / "SecondaryMesh.lumen", LumenEngine::GMultiBlockDLSL, LumenEngine::GMinimalMeshDLSL },
            { AssetsRoot / "Shaders" / "StressVertex.vert", GStressVertexShader, LumenEngine::Compiler::GMinimalVertexShader },
            { AssetsRoot / "Shaders" / "StressFragment.frag", GStressFragmentShader, LumenEngine::Compiler::GMinimalFragmentShader },
            { AssetsRoot / "Shaders" / "StressCompute.comp", GStressComputeShader, LumenEngine::Compiler::GComputeShader },
        };

        for ( const FAssetFileSpec &Spec : AssetFiles )
        {
            ASSERT_TRUE( WriteAssetFile( Spec.Path, Spec.PrimaryContent ) );
        }

        AssetCompiler = LumenEngine::MakeUnique<LumenEngine::Engine::FAssetCompiler>();
        AssetCompiler->Initialize( AssetsRoot.ToString() );
        AssetCompiler->SetOnAssetReloadedCallback(
            [this] ( const LumenEngine::FString &, const LumenEngine::Compiler::EAssetType::Type InType ) -> void
            {
                switch ( InType )
                {
                case LumenEngine::Compiler::EAssetType::Mesh:
                    ++MeshReloadCount;
                    break;
                case LumenEngine::Compiler::EAssetType::Material:
                    ++MaterialReloadCount;
                    break;
                case LumenEngine::Compiler::EAssetType::Shader:
                    ++ShaderReloadCount;
                    break;
                default:
                    break;
                }

                ++TotalReloadCount;
            } );
    }

    void TearDown () override
    {
        if ( bHasOriginalCurrentDirectory and bChangedCurrentDirectory )
        {
            EXPECT_TRUE( LumenEngine::FFileSystem::SetCurrentDirectory( OriginalCurrentDirectory ).has_value() );
        }

        if ( LumenEngine::Filesystem::FDirectory::Exists( TestRoot ) )
        {
            EXPECT_TRUE( LumenEngine::Filesystem::FDirectory::Delete( TestRoot, true ).has_value() );
        }
    }

protected:

    LumenEngine::Filesystem::FPath TestRoot;
    LumenEngine::Filesystem::FPath AssetsRoot;
    LumenEngine::Filesystem::FPath OriginalCurrentDirectory;
    LumenEngine::Bool bHasOriginalCurrentDirectory = false;
    LumenEngine::Bool bChangedCurrentDirectory     = false;
    LumenEngine::TUniquePtr<LumenEngine::Engine::FAssetCompiler> AssetCompiler;

    LumenEngine::TAtomic<LumenEngine::UInt32> TotalReloadCount    = 0;
    LumenEngine::TAtomic<LumenEngine::UInt32> MeshReloadCount     = 0;
    LumenEngine::TAtomic<LumenEngine::UInt32> MaterialReloadCount = 0;
    LumenEngine::TAtomic<LumenEngine::UInt32> ShaderReloadCount   = 0;
};

} // namespace

TEST_F( FAssetCompilerHotReloadFixture, ConcurrentMutationsTriggerHotReload )
{
    constexpr LumenEngine::USize WriterCount     = 7;
    constexpr LumenEngine::USize WriteIterations = 12;

    const LumenEngine::TVector<FAssetFileSpec> AssetFiles = {
        { AssetsRoot / "Materials" / "PrimaryMaterial.lumen", LumenEngine::GMinimalMaterialDLSL, LumenEngine::GMultiBlockDLSL },
        { AssetsRoot / "Materials" / "SecondaryMaterial.lumen", LumenEngine::GMultiBlockDLSL, LumenEngine::GMinimalMaterialDLSL },
        { AssetsRoot / "Meshes" / "PrimaryMesh.lumen", LumenEngine::GMinimalMeshDLSL, LumenEngine::GMultiBlockDLSL },
        { AssetsRoot / "Meshes" / "SecondaryMesh.lumen", LumenEngine::GMultiBlockDLSL, LumenEngine::GMinimalMeshDLSL },
        { AssetsRoot / "Shaders" / "StressVertex.vert", GStressVertexShader, LumenEngine::Compiler::GMinimalVertexShader },
        { AssetsRoot / "Shaders" / "StressFragment.frag", GStressFragmentShader, LumenEngine::Compiler::GMinimalFragmentShader },
        { AssetsRoot / "Shaders" / "StressCompute.comp", GStressComputeShader, LumenEngine::Compiler::GComputeShader },
    };

    std::barrier StartBarrier( static_cast<std::ptrdiff_t>( WriterCount + 1U ) );
    LumenEngine::TAtomic<LumenEngine::Bool> bAllWritesSucceeded = true;
    LumenEngine::TVector<std::thread> Writers;
    Writers.reserve( AssetFiles.size() );

    for ( const FAssetFileSpec &Spec : AssetFiles )
    {
        Writers.emplace_back(
            [&, Spec] () -> void
            {
                StartBarrier.arrive_and_wait();

                for ( LumenEngine::USize Iteration = 0U; Iteration < WriteIterations; ++Iteration )
                {
                    const LumenEngine::AnsiChar *Content = ( Iteration % 2U == 0U ) ? Spec.PrimaryContent : Spec.SecondaryContent;
                    if ( not WriteAssetFile( Spec.Path, Content ) )
                    {
                        bAllWritesSucceeded.store( false, std::memory_order_release );
                        return;
                    }

                    std::this_thread::sleep_for( std::chrono::milliseconds( 150 ) );
                }
            } );
    }

    StartBarrier.arrive_and_wait();

    std::this_thread::sleep_for( std::chrono::milliseconds( 1200 ) );
    AssetCompiler->Tick();

    std::this_thread::sleep_for( std::chrono::milliseconds( 1200 ) );
    AssetCompiler->Tick();

    for ( std::thread &Writer : Writers )
    {
        Writer.join();
    }

    EXPECT_TRUE( bAllWritesSucceeded.load( std::memory_order_acquire ) );
    EXPECT_GE( TotalReloadCount.load( std::memory_order_acquire ), WriterCount );
    EXPECT_GT( MeshReloadCount.load( std::memory_order_acquire ), 0U );
    EXPECT_GT( MaterialReloadCount.load( std::memory_order_acquire ), 0U );
    EXPECT_GT( ShaderReloadCount.load( std::memory_order_acquire ), 0U );
}
