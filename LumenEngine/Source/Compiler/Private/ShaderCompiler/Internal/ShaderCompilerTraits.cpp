/**
 * @file ShaderCompilerTraits.cpp
 * @brief Implementation for the shader compiler traits.
 */

#include "ShaderCompiler/Internal/ShaderCompilerTraits.hpp"
#include "Container/String.hpp"
#include "Container/Vector.hpp"
#include "ShaderCompiler/ShaderCompilerTypes.hpp"
#include <format>

LumenEngine::FString LumenEngine::Compiler::FShaderCompilerTraits::GetSourcePath ( const FShaderCompileRequest &InRequest ) noexcept
{
    return InRequest.SourcePath;
}

LumenEngine::FString LumenEngine::Compiler::FShaderCompilerTraits::BuildCachePath ( const Filesystem::FPath &InCacheDir,
                                                                                    FSourceHash InHash,
                                                                                    const FShaderCompileRequest &InRequest,
                                                                                    const AnsiChar *InExt ) noexcept
{
    return ( InCacheDir / std::format( "{:016x}_{}{}", InHash, EShaderStage::ToString( InRequest.Stage ), InExt ) ).ToString();
}

LumenEngine::Bool
LumenEngine::Compiler::FShaderCompilerTraits::IsValidMeta ( const FShaderCacheMetaData &InMeta, FSourceHash InHash, const FShaderCompileRequest &InRequest ) noexcept
{
    return InMeta.SourceHash == InHash and InMeta.Stage == InRequest.Stage;
}

LumenEngine::Compiler::FCompiledShader
LumenEngine::Compiler::FShaderCompilerTraits::RestoreFromCache ( FSourceHash InHash, const FShaderCacheMetaData &InMeta, TVector<FSpirvWord> &&InBinary ) noexcept
{
    return { .SpirV = std::move( InBinary ), .Reflection = {}, .Stage = InMeta.Stage, .Hash = InHash, .bFromCache = true, .EntryPoint = InMeta.EntryPoint };
}

LumenEngine::Compiler::FShaderCacheMetaData
LumenEngine::Compiler::FShaderCompilerTraits::CreateMeta ( FSourceHash InHash, const FShaderCompileRequest &InRequest, const FCompiledShader &InCompiled ) noexcept
{
    return { .SourceHash     = InHash,
             .Stage          = InRequest.Stage,
             .Optimization   = InRequest.Optimization,
             .CompiledAtNs   = 0, // Set by TCompilerCache
             .SpirVWordCount = static_cast<UInt32>( InCompiled.SpirV.size() ),
             .EntryPoint     = InRequest.EntryPoint };
}

const LumenEngine::TVector<LumenEngine::Compiler::FSpirvWord> &LumenEngine::Compiler::FShaderCompilerTraits::GetBinaryData ( const FCompiledShader &InCompiled ) noexcept
{
    return InCompiled.SpirV;
}