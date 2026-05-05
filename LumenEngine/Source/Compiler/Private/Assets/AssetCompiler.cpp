/**
 * @file AssetCompiler.cpp
 * @brief Implementation of the FAssetCompiler orchestration class.
 */

#include "Assets/AssetCompiler.hpp"
#include "Assets/AssetDeserializer.hpp"
#include "Assets/HotReload.hpp"

#include "Graphics/Renderer.hpp"
#include "Logging/Logger.hpp"

#include <filesystem>

LUMEN_LOG_DEFINE_CATEGORY( LogAssetCompiler, "AssetCompiler" );

/**
 * FAssetCompileResult
 */

LumenEngine::Bool LumenEngine::Compiler::FAssetCompileResult::IsSuccess () const noexcept
{
    return FailureCount == 0;
}

/**
 * Ctor & Dtor
 */

LumenEngine::Compiler::FAssetCompiler::FAssetCompiler () noexcept : ShaderCompiler( FShaderCompilerConfig() )
{
    /* Ctor */
}

LumenEngine::Compiler::FAssetCompiler::~FAssetCompiler () noexcept = default;

/**
 * Public
 */

void LumenEngine::Compiler::FAssetCompiler::Initialize ( const FString &InAssetsPath ) noexcept
{
    AssetsPath = InAssetsPath;

    if ( not CompileAll( AssetsPath ).IsSuccess() )
    {
        LUMEN_LOG_ERROR( LogAssetCompiler, "Initial asset compilation encountered errors." );
    }
    else
    {
        LUMEN_LOG_INFO( LogAssetCompiler, "Initial asset compilation successful." );
    }

    HotReload = MakeUnique<FCompilerHotReload>( *this, AssetsPath );
    HotReload->SetOnAssetReloadedCallback( [this] ( const FString &InPath, const EAssetType::Type InType ) -> void { AssetOnReloadCallback( InPath, InType ); } );
}

void LumenEngine::Compiler::FAssetCompiler::Tick () noexcept
{
    if ( HotReload )
    {
        HotReload->Tick();
    }
}

LumenEngine::TSharedPtr<LumenEngine::Renderer::FRenderMesh> LumenEngine::Compiler::FAssetCompiler::LoadMesh ( const FString &InName ) noexcept
{
    if ( MeshCache.contains( InName ) )
    {
        return MeshCache[InName];
    }

    const FString FilePath = AssetsPath + "/Meshes/" + InName + ".lumen";

    FLumenCompileRequest Request;
    Request.SourcePath        = FilePath;
    Request.ExpectedBlockType = "Mesh";

    if ( const FLumenCompileResult Result = LumenCompiler.CompileAsset( Request ); Result.IsSuccess() )
    {
        if ( const TOptional<FDeserializedMesh> Deserialized = FAssetDeserializer::DeserializeMesh( Result.Asset->BinaryBlob ) )
        {
            TSharedPtr<Renderer::FRenderMesh> Mesh = MakeShared<Renderer::FRenderMesh>();
            Mesh->Vertices                         = Deserialized->Vertices;
            Mesh->Indices                          = Deserialized->Indices;
            Mesh->RenderHandle                     = Renderer::GRenderer->CreateMesh( Mesh->Vertices, Mesh->Indices );

            MeshCache[InName] = Mesh;
            return Mesh;
        }
    }

    LUMEN_LOG_ERROR( LogAssetCompiler, "Failed to load mesh: {}", InName.c_str() );
    return nullptr;
}

LumenEngine::TSharedPtr<LumenEngine::Renderer::FRenderMaterial> LumenEngine::Compiler::FAssetCompiler::LoadMaterial ( const FString &InName ) noexcept
{
    if ( MaterialCache.contains( InName ) )
    {
        return MaterialCache[InName];
    }

    if ( not ShaderCache.contains( InName ) )
    {
        TSharedPtr<Renderer::FRenderShader> Shader = MakeShared<Renderer::FRenderShader>();
        Shader->VertexPath                         = AssetsPath + "/Shaders/" + InName + ".vert";
        Shader->FragmentPath                       = AssetsPath + "/Shaders/" + InName + ".frag";
        Shader->RenderHandle                       = Renderer::GRenderer->CreatePipeline( Shader->VertexPath, Shader->FragmentPath );
        ShaderCache[InName]                        = Shader;
    }

    TSharedPtr<Renderer::FRenderMaterial> Material = MakeShared<Renderer::FRenderMaterial>();
    Material->Shader                               = ShaderCache[InName];
    Material->RenderHandle                         = Renderer::GRenderer->CreateMaterial( Material->Shader );

    MaterialCache[InName] = Material;
    return Material;
}

LumenEngine::Compiler::FAssetCompileResult LumenEngine::Compiler::FAssetCompiler::CompileAll ( const FString &InAssetsPath ) noexcept
{
    FAssetCompileResult Result;
    const std::filesystem::path Root( InAssetsPath.c_str() );

    if ( not std::filesystem::exists( Root ) )
    {
        LUMEN_LOG_ERROR( LogAssetCompiler, "Assets path does not exist: {}", InAssetsPath.c_str() );
        Result.FailedFiles.emplace_back( InAssetsPath );
        Result.FailureCount++;
        return Result;
    }

    LUMEN_LOG_INFO( LogAssetCompiler, "Starting bulk compilation for Assets at: {}", InAssetsPath.c_str() );

    const FAssetCompileResult MaterialsResult = CompileFolder( ( Root / "Materials" ).string(), EAssetType::Material );
    Result.SuccessCount += MaterialsResult.SuccessCount;
    Result.FailureCount += MaterialsResult.FailureCount;
    Result.FailedFiles.insert( Result.FailedFiles.end(), MaterialsResult.FailedFiles.begin(), MaterialsResult.FailedFiles.end() );

    const FAssetCompileResult MeshesResult = CompileFolder( ( Root / "Meshes" ).string(), EAssetType::Mesh );
    Result.SuccessCount += MeshesResult.SuccessCount;
    Result.FailureCount += MeshesResult.FailureCount;
    Result.FailedFiles.insert( Result.FailedFiles.end(), MeshesResult.FailedFiles.begin(), MeshesResult.FailedFiles.end() );

    const FAssetCompileResult ShadersResult = CompileFolder( ( Root / "Shaders" ).string(), EAssetType::Unknown );
    Result.SuccessCount += ShadersResult.SuccessCount;
    Result.FailureCount += ShadersResult.FailureCount;
    Result.FailedFiles.insert( Result.FailedFiles.end(), ShadersResult.FailedFiles.begin(), ShadersResult.FailedFiles.end() );

    LUMEN_LOG_INFO( LogAssetCompiler, "Bulk compilation finished. Success: {}, Failure: {}", Result.SuccessCount, Result.FailureCount );

    return Result;
}

LumenEngine::Compiler::FAssetCompileResult LumenEngine::Compiler::FAssetCompiler::CompileFolder ( const FString &InFolderPath, EAssetType::Type InAssetType ) noexcept
{
    FAssetCompileResult Result;
    const std::filesystem::path Path( InFolderPath.c_str() );

    if ( not std::filesystem::exists( Path ) )
    {
        return Result;
    }

    for ( const auto &Entry : std::filesystem::directory_iterator( Path ) )
    {
        if ( not Entry.is_regular_file() )
        {
            continue;
        }

        const FAssetCompileResult FileResult = CompileFile( Entry.path().string(), InAssetType );
        Result.SuccessCount += FileResult.SuccessCount;
        Result.FailureCount += FileResult.FailureCount;
        Result.FailedFiles.insert( Result.FailedFiles.end(), FileResult.FailedFiles.begin(), FileResult.FailedFiles.end() );
    }

    return Result;
}

LumenEngine::Compiler::FAssetCompileResult LumenEngine::Compiler::FAssetCompiler::CompileFile ( const FString &InFilePath, EAssetType::Type InAssetType ) noexcept
{
    FAssetCompileResult Result;
    const std::filesystem::path Path( InFilePath.c_str() );
    const FString Extension = Path.extension().string();

    if ( InAssetType == EAssetType::Material or InAssetType == EAssetType::Mesh )
    {
        if ( Extension != ".lumen" )
        {
            return Result;
        }

        FLumenCompileRequest Request;
        Request.SourcePath        = InFilePath;
        Request.ExpectedBlockType = ( InAssetType == EAssetType::Material ) ? "Material" : "Mesh";

        if ( const FLumenCompileResult CompileRes = LumenCompiler.CompileAsset( Request ); CompileRes.IsSuccess() )
        {
            Result.SuccessCount++;
        }
        else
        {
            Result.FailureCount++;
            Result.FailedFiles.emplace_back( InFilePath );
            LUMEN_LOG_ERROR( LogAssetCompiler, "Failed to compile Lumen Asset {}: {}", InFilePath.c_str(), CompileRes.ErrorLog.c_str() );
        }
    }
    else if ( InAssetType == EAssetType::Shader or InAssetType == EAssetType::Unknown )
    {
        EShaderStage::Type Stage = EShaderStage::Count;
        if ( Extension == ".vert" )
        {
            Stage = EShaderStage::Vertex;
        }
        else if ( Extension == ".frag" )
        {
            Stage = EShaderStage::Fragment;
        }
        else if ( Extension == ".comp" )
        {
            Stage = EShaderStage::Compute;
        }
        else
        {
            return Result;
        }

        FShaderCompileRequest Request;
        Request.SourcePath = InFilePath;
        Request.Stage      = Stage;

        if ( const FShaderCompileResult CompileRes = ShaderCompiler.CompileShader( Request ); CompileRes.IsSuccess() )
        {
            Result.SuccessCount++;
        }
        else
        {
            Result.FailureCount++;
            Result.FailedFiles.emplace_back( InFilePath );
            LUMEN_LOG_ERROR( LogAssetCompiler, "Failed to compile shader {}: {}", InFilePath.c_str(), CompileRes.ErrorLog.c_str() );
        }
    }

    return Result;
}

/**
 * Private
 */

void LumenEngine::Compiler::FAssetCompiler::AssetOnReloadCallback ( const FString &InPath, const EAssetType::Type InType ) noexcept
{
    LUMEN_LOG_INFO( LogAssetCompiler, "Hot-Reloading asset: {} (Type: {})", InPath.c_str(), EAssetType::ToString( InType ) );

    switch ( InType )
    {
    case EAssetType::Shader:
        MaterialCache.clear();
        ShaderCache.clear();
        break;
    case EAssetType::Mesh:
        MeshCache.clear();
        break;
    default:
        break;
    }
}
