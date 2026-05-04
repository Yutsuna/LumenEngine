/**
 * @file ShaderCompiler.cpp
 * @brief Orchestration of compilation through internal subsystems.
 */

#include "ShaderCompiler/ShaderCompiler.hpp"

#include "ShaderCompiler/Internal/GlslangBackend.hpp"
#include "ShaderCompiler/Internal/HashUtils.hpp"
#include "ShaderCompiler/Internal/SpirvReflector.hpp"
#include "ShaderCompiler/Internal/SpirvUtils.hpp"
#include "ShaderCompiler/ShaderCompilerTypes.hpp"

#include "Container/File.hpp"
#include "Logging/Logger.hpp"

#include <format>

namespace LumenEngine::Compiler
{

/**
 * Ctor & Dtor
 */

FShaderCompiler::FShaderCompiler ( FShaderCompilerConfig InConfig ) noexcept : TCompiler( std::move( InConfig ) ), bInitialised( Internal::FGlslangBackend::Initialize() )
{
    /* Ctor */
}

FShaderCompiler::~FShaderCompiler ()
{
    if ( bInitialised )
    {
        Internal::FGlslangBackend::Finalize();
    }
}

/**
 * Shader Compilation
 */

FShaderCompileResult FShaderCompiler::CompileShader ( const FShaderCompileRequest &InRequest ) noexcept
{
    return TCompiler::Compile( InRequest );
}

FShaderCompileResult FShaderCompiler::CompileShaderFromSource ( FStringView InSource, const FShaderCompileRequest &InRequest ) noexcept
{
    if ( not bInitialised )
    {
        return FShaderCompileResult::Failure( EShaderCompilerError::GlslangInitFailed, "ShaderCompiler backend failed to initialize." );
    }

    return TCompiler::CompileFromSource( InSource, InRequest );
}

/**
 * Cache Management
 */

USize FShaderCompiler::WarmCache () noexcept
{
    USize LoadedCount = 0ULL;
    std::error_code Ec;

    if ( not std::filesystem::exists( Config.CacheDirectory, Ec ) )
    {
        return 0ULL;
    }

    for ( const auto &Entry : std::filesystem::directory_iterator( Config.CacheDirectory, Ec ) )
    {

        if ( Ec )
        {
            LUMEN_LOG_ERROR( LogShaderCompiler, "Error accessing cache directory entry: {}", Ec.message() );
            break;
        }

        if ( Entry.path().extension() == ".meta" )
        {
            if ( const TOptional<TVector<Byte>> MetaBytes = FIOFile::ReadAllBytes<Byte>( Entry.path().string() ) )
            {
                if ( const TOptional<FShaderCacheMetaData> MetaOpt = FShaderCacheMetaData::Deserialize( std::span<const Byte>( *MetaBytes ) ) )
                {
                    FShaderCompileRequest Request;
                    Request.Stage      = MetaOpt->Stage;
                    Request.EntryPoint = MetaOpt->EntryPoint;

                    if ( Cache->TryGet( MetaOpt->SourceHash, Request ) )
                    {
                        ++LoadedCount;
                    }
                }
            }
        }
    }
    return LoadedCount;
}

USize FShaderCompiler::ClearCache () noexcept
{
    return Cache->Clear();
}

USize FShaderCompiler::ClearStaleCache ( Float64 MaxAgeSeconds ) noexcept
{
    return Cache->ClearStale( MaxAgeSeconds );
}

/**
 * Getters
 */

Bool FShaderCompiler::IsReady () const noexcept
{
    return bInitialised;
}

/**
 * Static Helpers
 */

FSourceHash FShaderCompiler::ComputeRequestHash ( FStringView InSource, const FShaderCompileRequest &InRequest ) noexcept
{
    return Internal::FHashUtils::ComputeRequestHash( InSource, InRequest );
}

FString FShaderCompiler::DisassembleSpirV ( const FSpirVBlob &InSpirV ) noexcept
{
    return Internal::FSpirvUtils::Disassemble( InSpirV );
}

FString FShaderCompiler::ValidateSpirV ( const FSpirVBlob &InSpirV ) noexcept
{
    return Internal::FSpirvUtils::Validate( InSpirV );
}

/**
 * TCompiler Hooks
 */

FSourceHash FShaderCompiler::ComputeHash ( FStringView InSource, const FShaderCompileRequest &InRequest ) noexcept
{
    return Internal::FHashUtils::ComputeRequestHash( InSource, InRequest );
}

Bool FShaderCompiler::TryReflect ( FCompiledShader &InCompiled, FString &OutErrorLog ) noexcept
{
    const Bool bResult = Internal::FSpirvReflector::Reflect( InCompiled.SpirV, InCompiled.Stage, InCompiled.Reflection, OutErrorLog );

    if ( bResult )
    {
        LUMEN_LOG_SHADER_TRACE( LogShaderCompiler, "SPIR-V reflection successful (Hash: {:016x}, Stage: {})", InCompiled.Hash,
                                EShaderStage::ToString( InCompiled.Stage ) );
    }
    else
    {
        LUMEN_LOG_ERROR( LogShaderCompiler, "SPIR-V reflection failed (Hash: {:016x}, Stage: {}): {}", InCompiled.Hash, EShaderStage::ToString( InCompiled.Stage ),
                         OutErrorLog );
    }
    return bResult;
}

Bool FShaderCompiler::CompileFresh (
    FStringView InSource, const FShaderCompileRequest &InRequest, FSourceHash InHash, FCompiledShader &OutCompiled, FString &OutErrorLog ) const noexcept
{
    FSpirVBlob SpirV;

    if ( not Internal::FGlslangBackend::Compile( InSource, InRequest, SpirV, OutErrorLog ) )
    {
        return false;
    }

    OutCompiled = { .SpirV = std::move( SpirV ), .Reflection = {}, .Stage = InRequest.Stage, .Hash = InHash, .bFromCache = false, .EntryPoint = InRequest.EntryPoint };

    DumpDebugArtifacts( InHash, OutCompiled );

    LUMEN_LOG_SHADER_TRACE( LogShaderCompiler, "Shader compiled successfully (Hash: {:016x}, Stage: {})", InHash, EShaderStage::ToString( InRequest.Stage ) );
    return true;
}

/**
 * Private Methods
 */

void FShaderCompiler::DumpDebugArtifacts ( FSourceHash InHash, const FCompiledShader &InShader ) const noexcept
{
    if ( not Config.bDumpAssembly )
    {
        return;
    }

    const std::filesystem::path CacheDir( Config.CacheDirectory );
    const FString AsmPath     = ( CacheDir / std::format( "{:016x}.spvasm", InHash ) ).string();
    const FString Disassembly = Internal::FSpirvUtils::Disassemble( InShader.SpirV );

    if ( not FIOFile::WriteAllText( AsmPath, Disassembly ) )
    {
        LUMEN_LOG_SHADER_TRACE( LogShaderCompiler, "Failed to write assembly file: {}", AsmPath );
    }
}

} // namespace LumenEngine::Compiler
