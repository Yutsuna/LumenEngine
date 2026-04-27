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

    /** 1. Try Cache */
    if ( TOptional<FCompiledShader> CachedShader = Cache->TryGet( Hash, InRequest.Stage, InRequest.EntryPoint ) )
    {
        Cache->RecordHit();

        /** INFO: We still perform reflection even on cached shaders to ensure the FCompiledShader is fully populated */
        FString ReflectionError;
        if ( not Internal::FSpirvReflector::Reflect( CachedShader->SpirV, InRequest.Stage, CachedShader->Reflection, ReflectionError ) )
        {
            return FShaderCompileResult::Failure( EShaderCompilerError::ReflectionFailed, ReflectionError );
        }

        return FShaderCompileResult::Success( std::move( *CachedShader ) );
    }

    Cache->RecordMiss();

    /** 2. Compile GLSL -> SPIR-V */
    FSpirVBlob SpirV;
    FString ErrorLog;

    if ( not Internal::FGlslangBackend::Compile( InSource, InRequest, SpirV, ErrorLog ) )
    {
        return FShaderCompileResult::Failure( EShaderCompilerError::CompilationFailed, std::move( ErrorLog ) );
    }

    /** 3. Reflect SPIR-V */
    FShaderReflection Reflection;
    if ( not Internal::FSpirvReflector::Reflect( SpirV, InRequest.Stage, Reflection, ErrorLog ) )
    {
        return FShaderCompileResult::Failure( EShaderCompilerError::ReflectionFailed, std::move( ErrorLog ) );
    }

    /** 4. Construct Result */
    FCompiledShader Compiled{ .SpirV      = std::move( SpirV ),
                              .Reflection = std::move( Reflection ),
                              .Stage      = InRequest.Stage,
                              .Hash       = Hash,
                              .bFromCache = false,
                              .EntryPoint = InRequest.EntryPoint };

    /** 5. Persist */
    Cache->Put( Hash, InRequest, Compiled );

    if ( Config.bDumpAssembly )
    {
        const FString AsmPath = ( std::filesystem::path( Config.CacheDirectory ) / std::format( "{:016X}.spvasm", Hash ) ).string();
        if ( not FIOFile::WriteAllText( AsmPath, Internal::FSpirvUtils::Disassemble( Compiled.SpirV ) ) )
        {
            LUMEN_LOG_ERROR( LogShaderCompiler, "Failed to write assembly file: {}", AsmPath );
        }
    }

    return FShaderCompileResult::Success( std::move( Compiled ) );
}

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