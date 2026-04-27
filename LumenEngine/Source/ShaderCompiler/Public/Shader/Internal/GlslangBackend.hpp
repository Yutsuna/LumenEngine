/**
 * @file GlslangBackend.hpp
 * @brief Internal GLSL to SPIR-V compilation backend.
 */

#pragma once

#include "CoreTypes.hpp"

#include "Shader/ShaderCompilerTypes.hpp"

namespace LumenEngine
{

namespace Internal
{

    /**
     * @class FGlslangBackend
     * @brief Internal backend for GLSL to SPIR-V compilation using glslang.
     */
    class FGlslangBackend final
    {
    public:

        /**
         * @brief Initialize the glslang library. Must be called before any compilation.
         * @return True if initialization was successful, false otherwise.
         */
        static Bool Initialize () noexcept;

        /**
         * @brief Finalize the glslang library. Should be called during application shutdown to clean up resources.
         */
        static void Finalize () noexcept;

    public:

        /**
         * @brief Compile GLSL source code to SPIR-V bytecode using glslang.
         * @param InSource GLSL source code as a string view.
         * @param InRequest Shader compile request parameters
         * @param OutSpirV Output parameter to receive the compiled SPIR-V bytecode on success.
         * @param OutErrorLog Output parameter to receive error messages if compilation fails.
         * @return True if compilation was successful, false otherwise.
         */
        [[nodiscard]] static Bool Compile ( FStringView InSource, const FShaderCompileRequest &InRequest, FSpirVBlob &OutSpirV, FString &OutErrorLog ) noexcept;
    };

} // namespace Internal

} // namespace LumenEngine
