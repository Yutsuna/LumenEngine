/**
 * @file ShaderCompiler.cpp
 * @brief Orchestration of compilation through internal subsystems.
 */

#include "Shader/ShaderCompiler.hpp"

#include "Container/File.hpp"
#include "Shader/Internal/GlslangBackend.hpp"
#include "Shader/Internal/HashUtils.hpp"
#include "Shader/Internal/ShaderCache.hpp"
#include "Shader/Internal/SpirvReflector.hpp"
#include "Shader/Internal/SpirvUtils.hpp"

LumenEngine::FShaderCompiler::FShaderCompiler ( FShaderCompilerConfig InConfig ) noexcept
    : Config( std::move( InConfig ) ), Cache( MakeUnique<Internal::FShaderCache>( Config ) )
{
    bInitialised = Internal::FGlslangBackend::Initialize();
    if ( not bInitialised and Config.ErrorCallback )
    {
        Config.ErrorCallback( "FShaderCompiler: glslang failed to initialize." );
    }
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
    if ( not bInitialised )
    {
        return FShaderCompileResult::Failure( EShaderCompilerError::GlslangInitFailed, "Compiler not initialised." );
    }

    if ( InRequest.Stage >= EShaderStage::Count )
    {
        return FShaderCompileResult::Failure( EShaderCompilerError::InvalidStage, "Invalid shader stage provided." );
    }

    TOptional<FString> SourceOpt = FIOFile::ReadAllText( InRequest.SourcePath );
    if ( not SourceOpt.has_value() )
    {
        return FShaderCompileResult::Failure( EShaderCompilerError::FileNotFound, std::format( "Failed to read '{}'", InRequest.SourcePath ) );
    }

    return CompileShaderFromSource( *SourceOpt, InRequest );
}

LumenEngine::FShaderCompileResult LumenEngine::FShaderCompiler::CompileShaderFromSource ( FStringView InSource, const FShaderCompileRequest &InRequest ) noexcept
{
    if ( not bInitialised )
    {
        return FShaderCompileResult::Failure( EShaderCompilerError::GlslangInitFailed, "Compiler not initialised." );
    }

    const FSourceHash Hash = Internal::FHashUtils::ComputeRequestHash( InSource, InRequest );

    if ( TOptional<FCompiledShader> CachedShader = Cache->TryGet( Hash, InRequest.Stage, InRequest.EntryPoint ) )
    {
        Cache->RecordHit();

        if ( Config.InfoCallback )
        {
            Config.InfoCallback( std::format( "Cache HIT '{}' (Hash: {:016X})", InRequest.SourcePath, Hash ) );
        }

        /** INFO: Ensure reflection succeeds even from cached bytecode */
        FString ReflectionError;
        if ( not Internal::FSpirvReflector::Reflect( CachedShader->SpirV, InRequest.Stage, CachedShader->Reflection, ReflectionError ) )
        {
            return FShaderCompileResult::Failure( EShaderCompilerError::ReflectionFailed, ReflectionError );
        }

        return FShaderCompileResult::Success( std::move( *CachedShader ) );
    }

    Cache->RecordMiss();

    if ( Config.InfoCallback )
    {
        Config.InfoCallback( std::format( "Cache MISS '{}' (Hash: {:016X})", InRequest.SourcePath, Hash ) );
    }

    FSpirVBlob SpirV;
    FString ErrorLog;

    if ( not Internal::FGlslangBackend::Compile( InSource, InRequest, SpirV, ErrorLog ) )
    {
        return FShaderCompileResult::Failure( EShaderCompilerError::CompilationFailed, std::move( ErrorLog ) );
    }

    if ( Config.MaxSpirVWords > 0ULL and SpirV.size() > Config.MaxSpirVWords )
    {
        return FShaderCompileResult::Failure( EShaderCompilerError::OptimizationFailed, "SPIR-V blob exceeds maximum allowed word count." );
    }

    FShaderReflection Reflection;
    if ( not Internal::FSpirvReflector::Reflect( SpirV, InRequest.Stage, Reflection, ErrorLog ) )
    {
        return FShaderCompileResult::Failure( EShaderCompilerError::ReflectionFailed, std::move( ErrorLog ) );
    }

    FCompiledShader Compiled{ .SpirV      = std::move( SpirV ),
                              .Reflection = std::move( Reflection ),
                              .Stage      = InRequest.Stage,
                              .Hash       = Hash,
                              .bFromCache = false,
                              .EntryPoint = InRequest.EntryPoint };

    Cache->Put( Hash, InRequest, Compiled );

    if ( Config.bDumpAssembly )
    {
        const FString AssemblyPath = std::format( "{}/{:016X}_{}.spvasm", Config.CacheDirectory.string(), Hash, EShaderStage::ToString( InRequest.Stage ) );
        if ( not FIOFile::WriteAllText( AssemblyPath, Internal::FSpirvUtils::Disassemble( Compiled.SpirV ) ) )
        {
            if ( Config.ErrorCallback )
            {
                Config.ErrorCallback( std::format( "Failed to write assembly file '{}'", AssemblyPath ) );
            }
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
    if ( TOptional<FString> SourceOpt = FIOFile::ReadAllText( InRequest.SourcePath ) )
    {
        const FSourceHash Hash = Internal::FHashUtils::ComputeRequestHash( *SourceOpt, InRequest );
        Cache->Invalidate( Hash, InRequest.Stage );
    }
}

LumenEngine::USize LumenEngine::FShaderCompiler::ClearCache () noexcept
{
    return Cache->Clear();
}

LumenEngine::USize LumenEngine::FShaderCompiler::ClearStaleCache ( const double MaxAgeSeconds ) noexcept
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