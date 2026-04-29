/**
 * @file ShaderCompilerTraits.hpp
 * @brief Traits definition for FShaderCompiler to use with TCompiler.
 */

#pragma once

#include "CoreTypes.hpp"

#include "ShaderCompiler/ShaderCompilerRequest.hpp"
#include "ShaderCompiler/ShaderCompilerTypes.hpp"

#include <filesystem>
#include <format>

namespace LumenEngine
{
namespace Compiler
{

    /**
     * @struct FShaderCompilerTraits
     * @brief Traits for the shader compiler implementation.
     */
    struct FShaderCompilerTraits
    {
        using RequestType    = FShaderCompileRequest;
        using ResultType     = FShaderCompileResult;
        using CompiledType   = FCompiledShader;
        using ConfigType     = FShaderCompilerConfig;
        using MetaType       = FShaderCacheMetaData;
        using BinaryWordType = FSpirvWord;

        static constexpr const AnsiChar *BinaryExtension = ".spv";

        [[nodiscard]] static FString GetSourcePath ( const FShaderCompileRequest &InRequest ) noexcept
        {
            return InRequest.SourcePath;
        }

        [[nodiscard]] static FString
        BuildCachePath ( const std::filesystem::path &InCacheDir, FSourceHash InHash, const FShaderCompileRequest &InRequest, const AnsiChar *InExt ) noexcept
        {
            // The hash already includes all relevant request parameters (Stage, EntryPoint, etc.)
            // so we don't strictly need them in the filename, but keeping the Stage for readability
            // like the original implementation did is a good idea for manual cache inspection.
            return ( InCacheDir / std::format( "{:016x}_{}{}", InHash, EShaderStage::ToString( InRequest.Stage ), InExt ) ).string();
        }

        [[nodiscard]] static Bool IsValidMeta ( const FShaderCacheMetaData &InMeta, FSourceHash InHash, const FShaderCompileRequest &InRequest ) noexcept
        {
            return InMeta.SourceHash == InHash and InMeta.Stage == InRequest.Stage;
        }

        [[nodiscard]] static FCompiledShader RestoreFromCache ( FSourceHash InHash,
                                                                [[maybe_unused]] const FShaderCompileRequest &InRequest,
                                                                const FShaderCacheMetaData &InMeta,
                                                                TVector<FSpirvWord> &&InBinary ) noexcept
        {
            return { .SpirV = std::move( InBinary ), .Reflection = {}, .Stage = InMeta.Stage, .Hash = InHash, .bFromCache = true, .EntryPoint = InMeta.EntryPoint };
        }

        [[nodiscard]] static FShaderCacheMetaData CreateMeta ( FSourceHash InHash, const FShaderCompileRequest &InRequest, const FCompiledShader &InCompiled ) noexcept
        {
            return { .SourceHash     = InHash,
                     .Stage          = InRequest.Stage,
                     .Optimization   = InRequest.Optimization,
                     .CompiledAtNs   = 0, // Set by TCompilerCache
                     .SpirVWordCount = static_cast<UInt32>( InCompiled.SpirV.size() ),
                     .EntryPoint     = InRequest.EntryPoint };
        }

        [[nodiscard]] static const TVector<FSpirvWord> &GetBinaryData ( const FCompiledShader &InCompiled ) noexcept
        {
            return InCompiled.SpirV;
        }
    };

} // namespace Compiler
} // namespace LumenEngine
