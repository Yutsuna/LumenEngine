/**
 * @file ShaderCompiler.hpp
 * @brief Public facade for runtime GLSL -> SPIR-V shader compilation with persistent cache.
 */

#pragma once

#include "CoreTypes.hpp"

#include "Container/String.hpp"

#include "ShaderCompiler/ShaderCompilerRequest.hpp"
#include "ShaderCompiler/ShaderCompilerTypes.hpp"

#include "CompilerCore/CompilerBase.hpp"
#include "ShaderCompiler/Internal/ShaderCompilerTraits.hpp"

namespace LumenEngine
{

namespace Compiler
{

    /**
     * @class FShaderCompiler
     * @brief A runtime GLSL to SPIR-V shader compiler with a persistent cache.
     * @details Strictly adheres to SRP. All third-party dependencies are hidden.
     */
    class LUMEN_ENGINE_API FShaderCompiler final : public TCompiler<FShaderCompilerTraits, FShaderCompiler>
    {
    public:

        FShaderCompiler () noexcept;
        explicit FShaderCompiler ( FShaderCompilerConfig InConfig ) noexcept;
        ~FShaderCompiler () override;

    public:

        /**
         * @brief Compile (or load from cache) a single shader stage from a file.
         * @param InRequest Shader compile request parameters.
         * @return Compilation result containing SPIR-V bytecode or error message.
         */
        [[nodiscard]] FShaderCompileResult CompileShader ( const FShaderCompileRequest &InRequest ) noexcept;

        /**
         * @brief Compile from an in-memory GLSL source string instead of an I/O file.
         * @param InSource GLSL source text.
         * @param InRequest Shader compile request parameters.
         * @return Compilation result containing SPIR-V bytecode or error message.
         */
        [[nodiscard]] FShaderCompileResult CompileShaderFromSource ( FStringView InSource, const FShaderCompileRequest &InRequest ) noexcept;

    public:

        /**
         * @brief Pre-warm the in-memory cache from the disk cache directory.
         * @return Number of entries successfully loaded.
         */
        [[nodiscard]] USize WarmCache () noexcept;

        /**
         * @brief Remove all cached files from the cache directory.
         * @return Number of file pairs removed.
         */
        [[nodiscard]] USize ClearCache () noexcept;

        /**
         * @brief Remove cache entries whose .spv files are older than MaxAgeSeconds.
         * @return Number of file pairs removed.
         */
        [[nodiscard]] USize ClearStaleCache ( Float64 MaxAgeSeconds ) noexcept;

    public:

        [[nodiscard]] Bool IsReady () const noexcept;

    public:

        [[nodiscard]] static FSourceHash ComputeRequestHash ( FStringView InSource, const FShaderCompileRequest &InRequest ) noexcept;
        [[nodiscard]] static FString DisassembleSpirV ( const FSpirVBlob &InSpirV ) noexcept;
        [[nodiscard]] static FString ValidateSpirV ( const FSpirVBlob &InSpirV ) noexcept;

    public:

        /** TCompiler Hooks */
        [[nodiscard]] static FSourceHash ComputeHash ( FStringView InSource, const FShaderCompileRequest &InRequest ) noexcept;
        [[nodiscard]] static Bool TryReflect ( FCompiledShader &InCompiled, FString &OutErrorLog ) noexcept;
        [[nodiscard]] Bool CompileFresh (
            FStringView InSource, const FShaderCompileRequest &InRequest, FSourceHash InHash, FCompiledShader &OutCompiled, FString &OutErrorLog ) const noexcept;

    private:

        void DumpDebugArtifacts ( FSourceHash InHash, const FCompiledShader &InShader ) const noexcept;

    private:

        Bool bInitialised = false;
    };

} // namespace Compiler

} // namespace LumenEngine
