/**
 * @file AssetCompiler.cpp
 * @brief Implementation of the FAssetCompiler orchestration class.
 */

#include "Assets/AssetCompiler.hpp"
#include "Assets/AssetDeserializer.hpp"
#include "Assets/HotReload.hpp"

#include "Filesystem/Directory.hpp"
#include "Filesystem/MemoryMappedFile.hpp"
#include "Filesystem/Path.hpp"

#include "Graphics/Renderer.hpp"
#include "Logging/Logger.hpp"

LUMEN_LOG_DEFINE_CATEGORY( LogAssetCompiler, "AssetCompiler" );

/**
 * FAssetCompileResult
 */

LumenEngine::Bool LumenEngine::Engine::FAssetCompileResult::IsSuccess () const noexcept
{
    return FailureCount == 0;
}

/**
 * Ctor & Dtor
 */

LumenEngine::Engine::FAssetCompiler::FAssetCompiler () noexcept = default;

LumenEngine::Engine::FAssetCompiler::~FAssetCompiler () noexcept = default;

/**
 * Public
 */

void LumenEngine::Engine::FAssetCompiler::Initialize ( const FString &InAssetsPath ) noexcept
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
    HotReload->SetOnAssetReloadedCallback( [this] ( const FString &InPath, const Compiler::EAssetType::Type InType ) -> void
                                           { AssetOnReloadCallback( InPath, InType ); } );
}

void LumenEngine::Engine::FAssetCompiler::Tick () noexcept
{
    if ( HotReload )
    {
        HotReload->Tick();
    }
}

void LumenEngine::Engine::FAssetCompiler::SetOnAssetReloadedCallback ( FOnAssetReloaded InCallback ) noexcept
{
    OnAssetReloaded = std::move( InCallback );
}

LumenEngine::TSharedPtr<LumenEngine::Renderer::FRenderMesh> LumenEngine::Engine::FAssetCompiler::LoadMesh ( const FString &InName ) noexcept
{
    if ( MeshCache.contains( InName ) )
    {
        return MeshCache[InName];
    }

    const FString FilePath = AssetsPath + "/Meshes/" + InName + ".lumen";

    Compiler::FLumenCompileRequest Request;
    Request.SourcePath        = FilePath;
    Request.ExpectedBlockType = "Mesh";
    Request.TargetBlockName   = InName;

    const Compiler::FAssetHash Hash = Compiler::FLumenCompiler::ComputeHash( "", Request );

    const FString CachePath = ( LumenCompiler.GetConfig().CacheDirectory / std::format( "{:016x}_{}.lumenbin", Hash, InName ) ).ToString();

    if ( auto MappedResult = Filesystem::FMemoryMappedFile::Open( CachePath ) )
    {
        TUniquePtr<Filesystem::FMemoryMappedFile> MappedFile = std::move( *MappedResult );

        if ( auto ViewResult = FAssetDeserializer::DeserializeMeshView( MappedFile->GetRegion() ) )
        {
            const FMeshView &View = *ViewResult;

            TSharedPtr<Renderer::FRenderMesh> Mesh = MakeShared<Renderer::FRenderMesh>();
            Mesh->Vertices.assign( View.Vertices.begin(), View.Vertices.end() );
            Mesh->Indices.assign( View.Indices.begin(), View.Indices.end() );

            Mesh->RenderHandle = Renderer::GRenderer->CreateMesh( Mesh->Vertices, Mesh->Indices );

            MeshCache[InName] = Mesh;
            return Mesh;
        }
    }

    if ( const Compiler::FLumenCompileResult Result = LumenCompiler.CompileAsset( Request ); Result.IsSuccess() )
    {
        if ( auto DeserializedResult = FAssetDeserializer::DeserializeMesh( Result.Asset->BinaryBlob ) )
        {
            const FDeserializedMesh &Deserialized  = *DeserializedResult;
            TSharedPtr<Renderer::FRenderMesh> Mesh = MakeShared<Renderer::FRenderMesh>();

            Mesh->Vertices     = Deserialized.Vertices;
            Mesh->Indices      = Deserialized.Indices;
            Mesh->RenderHandle = Renderer::GRenderer->CreateMesh( Mesh->Vertices, Mesh->Indices );

            MeshCache[InName] = Mesh;
            return Mesh;
        }
    }

    LUMEN_LOG_ERROR( LogAssetCompiler, "Failed to load mesh: {}", InName.c_str() );
    return nullptr;
}

LumenEngine::TSharedPtr<LumenEngine::Renderer::FRenderMaterial> LumenEngine::Engine::FAssetCompiler::LoadMaterial ( const FString &InName ) noexcept
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

LumenEngine::Engine::FAssetCompileResult LumenEngine::Engine::FAssetCompiler::CompileAll ( const FString &InAssetsPath ) noexcept
{
    FAssetCompileResult Result;
    const Filesystem::FPath Root( InAssetsPath );

    if ( not Filesystem::FDirectory::Exists( Root ) )
    {
        LUMEN_LOG_ERROR( LogAssetCompiler, "Assets path does not exist: {}", InAssetsPath.c_str() );
        Result.FailedFiles.emplace_back( InAssetsPath );
        ++Result.FailureCount;
        return Result;
    }

    LUMEN_LOG_INFO( LogAssetCompiler, "Starting bulk compilation for Assets at: {}", InAssetsPath.c_str() );

    const FAssetCompileResult MaterialsResult = CompileFolder( ( Root / "Materials" ).ToString(), Compiler::EAssetType::Material );
    Result.SuccessCount += MaterialsResult.SuccessCount;
    Result.FailureCount += MaterialsResult.FailureCount;
    Result.FailedFiles.insert( Result.FailedFiles.end(), MaterialsResult.FailedFiles.begin(), MaterialsResult.FailedFiles.end() );

    const FAssetCompileResult MeshesResult = CompileFolder( ( Root / "Meshes" ).ToString(), Compiler::EAssetType::Mesh );
    Result.SuccessCount += MeshesResult.SuccessCount;
    Result.FailureCount += MeshesResult.FailureCount;
    Result.FailedFiles.insert( Result.FailedFiles.end(), MeshesResult.FailedFiles.begin(), MeshesResult.FailedFiles.end() );

    const FAssetCompileResult ShadersResult = CompileFolder( ( Root / "Shaders" ).ToString(), Compiler::EAssetType::Unknown );
    Result.SuccessCount += ShadersResult.SuccessCount;
    Result.FailureCount += ShadersResult.FailureCount;
    Result.FailedFiles.insert( Result.FailedFiles.end(), ShadersResult.FailedFiles.begin(), ShadersResult.FailedFiles.end() );

    LUMEN_LOG_INFO( LogAssetCompiler, "Bulk compilation finished. Success: {}, Failure: {}", Result.SuccessCount, Result.FailureCount );

    return Result;
}

LumenEngine::Engine::FAssetCompileResult LumenEngine::Engine::FAssetCompiler::CompileFolder ( const FString &InFolderPath,
                                                                                              Compiler::EAssetType::Type InAssetType ) noexcept
{
    FAssetCompileResult Result;
    const Filesystem::FPath Path( InFolderPath );

    if ( not Filesystem::FDirectory::Exists( Path ) )
    {
        return Result;
    }

    auto FilesResult = Filesystem::FDirectory::GetFiles( Path );
    if ( not FilesResult )
    {
        return Result;
    }

    for ( const auto &FileInfo : *FilesResult )
    {
        if ( FileInfo.IsDirectory() )
        {
            continue;
        }

        const FAssetCompileResult FileResult = CompileFile( FileInfo.Path, InAssetType );
        Result.SuccessCount += FileResult.SuccessCount;
        Result.FailureCount += FileResult.FailureCount;
        Result.FailedFiles.insert( Result.FailedFiles.end(), FileResult.FailedFiles.begin(), FileResult.FailedFiles.end() );
    }

    return Result;
}

LumenEngine::Engine::FAssetCompileResult LumenEngine::Engine::FAssetCompiler::CompileFile ( const FString &InFilePath, Compiler::EAssetType::Type InAssetType ) noexcept
{
    FAssetCompileResult Result;
    const Filesystem::FPath Path( InFilePath );
    const FString Extension = Path.GetExtension();

    if ( InAssetType == Compiler::EAssetType::Material or InAssetType == Compiler::EAssetType::Mesh )
    {
        if ( Extension != ".lumen" )
        {
            return Result;
        }

        Compiler::FLumenCompileRequest Request;
        Request.SourcePath        = InFilePath;
        Request.ExpectedBlockType = ( InAssetType == Compiler::EAssetType::Material ) ? "Material" : "Mesh";

        if ( const Compiler::FLumenCompileResult CompileRes = LumenCompiler.CompileAsset( Request ); CompileRes.IsSuccess() )
        {
            ++Result.SuccessCount;
        }
        else
        {
            ++Result.FailureCount;
            Result.FailedFiles.emplace_back( InFilePath );
            LUMEN_LOG_ERROR( LogAssetCompiler, "Failed to compile Lumen Asset {}: {}", InFilePath.c_str(), CompileRes.ErrorLog.c_str() );
        }
    }
    else if ( InAssetType == Compiler::EAssetType::Shader or InAssetType == Compiler::EAssetType::Unknown )
    {
        Compiler::EShaderStage::Type Stage = Compiler::EShaderStage::Count;
        if ( Extension == ".vert" )
        {
            Stage = Compiler::EShaderStage::Vertex;
        }
        else if ( Extension == ".frag" )
        {
            Stage = Compiler::EShaderStage::Fragment;
        }
        else if ( Extension == ".comp" )
        {
            Stage = Compiler::EShaderStage::Compute;
        }
        else
        {
            return Result;
        }

        Compiler::FShaderCompileRequest Request;
        Request.SourcePath = InFilePath;
        Request.Stage      = Stage;

        if ( const Compiler::FShaderCompileResult CompileRes = ShaderCompiler.CompileShader( Request ); CompileRes.IsSuccess() )
        {
            ++Result.SuccessCount;
        }
        else
        {
            ++Result.FailureCount;
            Result.FailedFiles.emplace_back( InFilePath );
            LUMEN_LOG_ERROR( LogAssetCompiler, "Failed to compile shader {}: {}", InFilePath.c_str(), CompileRes.ErrorLog.c_str() );
        }
    }

    return Result;
}

/**
 * Private
 */

void LumenEngine::Engine::FAssetCompiler::AssetOnReloadCallback ( const FString &InPath, const Compiler::EAssetType::Type InType ) noexcept
{
    LUMEN_LOG_INFO( LogAssetCompiler, "Hot-Reloading asset: {} (Type: {})", InPath.c_str(), Compiler::EAssetType::ToString( InType ) );

    switch ( InType )
    {
    case Compiler::EAssetType::Mesh:
        MeshCache.clear();
        break;
    case Compiler::EAssetType::Material:
        MaterialCache.clear();
        break;
    case Compiler::EAssetType::Shader:
        MaterialCache.clear();
        ShaderCache.clear();
        break;

    default:
        break;
    }

    if ( OnAssetReloaded )
    {
        OnAssetReloaded( InPath, InType );
    }
}
