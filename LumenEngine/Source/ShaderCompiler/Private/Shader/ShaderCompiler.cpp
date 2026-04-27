/**
 * @file ShaderCompiler.cpp
 * @brief Orchestration of compilation through internal subsystems.
 */

#include "Shader/ShaderCompiler.hpp"

#include "Shader/Internal/GlslangBackend.hpp"
#include "Shader/Internal/HashUtils.hpp"
#include "Shader/Internal/ShaderCache.hpp"
#include "Shader/Internal/SpirvReflector.hpp"
#include "Shader/Internal/SpirvUtils.hpp"
#include "Shader/ShaderCompilerTypes.hpp"

#include "Container/File.hpp"

#include <format>

/**
 * Ctor & Dtor
 */

LumenEngine::FShaderCompiler::FShaderCompiler ( FShaderCompilerConfig InConfig ) noexcept
    : Config( std::move( InConfig ) ), Cache( MakeUnique<Internal::FShaderCache>( Config ) ), bInitialised( Internal::FGlslangBackend::Initialize() )

{
    /* Ctor */
}

LumenEngine::FShaderCompiler::~FShaderCompiler () noexcept
{
    if ( bInitialised )
    {
        Internal::FGlslangBackend::Finalize();
    }
}

/**
 * Shader Compilation
 */

LumenEngine::FShaderCompileResult LumenEngine::FShaderCompiler::CompileShader ( const FShaderCompileRequest &InRequest ) noexcept
{
    TOptional<FString> SourceOpt = FIOFile::ReadAllText( InRequest.SourcePath );
    if ( not SourceOpt.has_value() )
    {
        return FShaderCompileResult::Failure( EShaderCompilerError::FileNotFound, std::format( "Failed to read source file: {}", InRequest.SourcePath ) );
    }

    return CompileShaderFromSource( *SourceOpt, InRequest );
}

LumenEngine::FShaderCompileResult LumenEngine::FShaderCompiler::CompileShaderFromSource ( FStringView InSource, const FShaderCompileRequest &InRequest ) noexcept
{
    if ( not bInitialised )
    {
        return FShaderCompileResult::Failure( EShaderCompilerError::GlslangInitFailed, "ShaderCompiler backend failed to initialize." );
    }

    const FSourceHash Hash = Internal::FHashUtils::ComputeRequestHash( InSource, InRequest );

    FShaderCompileResult CacheResult = TryLoadFromCache( InRequest, Hash );
    if ( CacheResult.IsSuccess() )
    {
        return CacheResult;
    }

    return CompileFreshAndCache( InSource, InRequest, Hash );
}

/**
 * Cache Management
 */

LumenEngine::USize LumenEngine::FShaderCompiler::WarmCache () noexcept
{
    return Cache->WarmUp();
}

void LumenEngine::FShaderCompiler::InvalidateCacheEntry ( const FShaderCompileRequest &InRequest ) noexcept
{
    if ( TOptional<FString> Source = FIOFile::ReadAllText( InRequest.SourcePath ) )
    {
        Cache->Invalidate( Internal::FHashUtils::ComputeRequestHash( *Source, InRequest ), InRequest.Stage );
    }
}

LumenEngine::USize LumenEngine::FShaderCompiler::ClearCache () noexcept
{
    return Cache->Clear();
}

LumenEngine::USize LumenEngine::FShaderCompiler::ClearStaleCache ( double MaxAgeSeconds ) noexcept
{
    return Cache->ClearStale( MaxAgeSeconds );
}

LumenEngine::UInt64 LumenEngine::FShaderCompiler::GetCacheHitCount () const noexcept
{
    return Cache->GetHits();
}

/**
 * Getters
 */

LumenEngine::UInt64 LumenEngine::FShaderCompiler::GetCacheMissCount () const noexcept
{
    return Cache->GetMisses();
}

LumenEngine::Bool LumenEngine::FShaderCompiler::IsReady () const noexcept
{
    return bInitialised;
}

const LumenEngine::FShaderCompilerConfig &LumenEngine::FShaderCompiler::GetConfig () const noexcept
{
    return Config;
}

/**
 * Static Helpers
 */

LumenEngine::FSourceHash LumenEngine::FShaderCompiler::ComputeRequestHash ( FStringView InSource, const FShaderCompileRequest &InRequest ) noexcept
{
    return Internal::FHashUtils::ComputeRequestHash( InSource, InRequest );
}

LumenEngine::FString LumenEngine::FShaderCompiler::DisassembleSpirV ( const FSpirVBlob &InSpirV ) noexcept
{
    return Internal::FSpirvUtils::Disassemble( InSpirV );
}

LumenEngine::FString LumenEngine::FShaderCompiler::ValidateSpirV ( const FSpirVBlob &InSpirV ) noexcept
{
    return Internal::FSpirvUtils::Validate( InSpirV );
}

/**
 * Private Methods
 */

namespace
{

inline LumenEngine::Bool TryReflect ( LumenEngine::FCompiledShader &InOutShader, LumenEngine::FString &OutError ) noexcept
{
    return LumenEngine::Internal::FSpirvReflector::Reflect( InOutShader.SpirV, InOutShader.Stage, InOutShader.Reflection, OutError );
}

inline LumenEngine::FCompiledShader
BuildShaderObject ( LumenEngine::FSpirVBlob &&InSpirV, const LumenEngine::FShaderCompileRequest &InRequest, LumenEngine::FSourceHash Hash ) noexcept
{
    return { .SpirV = std::move( InSpirV ), .Reflection = {}, .Stage = InRequest.Stage, .Hash = Hash, .bFromCache = false, .EntryPoint = InRequest.EntryPoint };
}

} // namespace

LumenEngine::FShaderCompileResult LumenEngine::FShaderCompiler::TryLoadFromCache ( const FShaderCompileRequest &InRequest, FSourceHash Hash ) const noexcept
{
    TOptional<FCompiledShader> Cached = Cache->TryGet( Hash, InRequest.Stage, InRequest.EntryPoint );
    if ( not Cached )
    {
        Cache->RecordMiss();
        return FShaderCompileResult::Failure( EShaderCompilerError::CacheReadFailed );
    }

    LUMEN_LOG_SHADER_TRACE( LogShaderCompiler, "Shader cache hit for hash {:016x} (Stage: {})", Hash, EShaderStage::ToString( InRequest.Stage ) );
    Cache->RecordHit();

    FString Error;
    if ( not TryReflect( *Cached, Error ) )
    {
        return FShaderCompileResult::Failure( EShaderCompilerError::ReflectionFailed, std::move( Error ) );
    }

    LUMEN_LOG_SHADER_TRACE( LogShaderCompiler, "Successfully loaded shader from cache (Hash: {:016x}, Stage: {})", Hash, EShaderStage::ToString( InRequest.Stage ) );
    return FShaderCompileResult::Success( std::move( *Cached ) );
}

LumenEngine::FShaderCompileResult
LumenEngine::FShaderCompiler::CompileFreshAndCache ( FStringView InSource, const FShaderCompileRequest &InRequest, FSourceHash Hash ) const noexcept
{
    FSpirVBlob SpirV;
    FString ErrorLog;

    /** INFO: GLSL -> SPIR-V */
    if ( not Internal::FGlslangBackend::Compile( InSource, InRequest, SpirV, ErrorLog ) )
    {
        return FShaderCompileResult::Failure( EShaderCompilerError::CompilationFailed, std::move( ErrorLog ) );
    }

    LUMEN_LOG_SHADER_TRACE( LogShaderCompiler, "Shader compiled successfully (Hash: {:016x}, Stage: {})", Hash, EShaderStage::ToString( InRequest.Stage ) );

    /** INFO: Build the compiled shader object */
    FCompiledShader Compiled = BuildShaderObject( std::move( SpirV ), InRequest, Hash );

    /** INFO: SPIR-V Reflection */
    if ( not TryReflect( Compiled, ErrorLog ) )
    {
        return FShaderCompileResult::Failure( EShaderCompilerError::ReflectionFailed, std::move( ErrorLog ) );
    }
    LUMEN_LOG_SHADER_TRACE( LogShaderCompiler, "SPIR-V reflection successful (Hash: {:016x}, Stage: {})", Hash, EShaderStage::ToString( InRequest.Stage ) );

    /** INFO: Cache the compiled shader for future use */
    Cache->Put( Hash, InRequest, Compiled );
    DumpDebugArtifacts( Hash, Compiled );

    LUMEN_LOG_SHADER_TRACE( LogShaderCompiler, "Shader compilation and caching complete (Hash: {:016x}, Stage: {})", Hash, EShaderStage::ToString( InRequest.Stage ) );
    return FShaderCompileResult::Success( std::move( Compiled ) );
}

void LumenEngine::FShaderCompiler::DumpDebugArtifacts ( FSourceHash Hash, const FCompiledShader &Shader ) const noexcept
{
    if ( not Config.bDumpAssembly )
    {
        return;
    }

    const std::filesystem::path CacheDir( Config.CacheDirectory );
    const FString AsmPath     = ( CacheDir / std::format( "{:016x}.spvasm", Hash ) ).string();
    const FString Disassembly = Internal::FSpirvUtils::Disassemble( Shader.SpirV );

    if ( not FIOFile::WriteAllText( AsmPath, Disassembly ) )
    {
        LUMEN_LOG_SHADER_TRACE( LogShaderCompiler, "Failed to write assembly file: {}", AsmPath );
    }
}
