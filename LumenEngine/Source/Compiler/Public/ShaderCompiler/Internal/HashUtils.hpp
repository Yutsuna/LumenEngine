/**
 * @file HashUtils.hpp
 * @brief Thread-safe, high-performance hashing utilities for shader compilation.
 */

#pragma once

#include "CoreTypes.hpp"

#include "ShaderCompiler/ShaderCompilerRequest.hpp"
#include "ShaderCompiler/ShaderCompilerTypes.hpp"

namespace LumenEngine
{

namespace Compiler
{

    namespace Internal
    {

        /**
         * @class FHashUtils
         * @brief Provides robust cryptographic-like hashing (FNV-1a 64-bit) for caching purposes.
         */
        class FHashUtils final
        {
        public:

            /**
             * @brief Computes a deterministic 64-bit hash for a shader compilation request.
             * @param InSource The raw GLSL source code.
             * @param InRequest The parameters affecting compilation.
             * @return A 64-bit FNV-1a hash.
             */
            [[nodiscard]] static FSourceHash ComputeRequestHash ( FStringView InSource, const FShaderCompileRequest &InRequest ) noexcept;

        private:

            static void CombineHash ( FSourceHash &InOutHash, const void *InData, USize InSize ) noexcept;
        };

    } // namespace Internal

} // namespace Compiler

} // namespace LumenEngine