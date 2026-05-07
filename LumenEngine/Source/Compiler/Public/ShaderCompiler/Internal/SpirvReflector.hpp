/**
 * @file SpirvReflector.hpp
 * @brief Handles metadata extraction from SPIR-V bytecode.
 */

#pragma once

#include "CoreTypes.hpp"
#include "ShaderCompiler/ShaderCompilerTypes.hpp"

namespace LumenEngine
{

namespace Compiler
{

    namespace Internal
    {

        /**
         * @class FSpirvReflector
         * @brief Safe wrapper around spirv-cross for extracting reflection data.
         */
        class FSpirvReflector final
        {
        public:

            /**
             * @brief Extracts resource bindings, inputs, and push constants from SPIR-V.
             * @param InSpirV The compiled SPIR-V bytecode.
             * @param InStage The shader stage for masking.
             * @param OutReflection Resulting reflection metadata.
             * @param OutError Error message if reflection fails.
             * @return True if extraction was successful.
             */
            [[nodiscard]] static Bool Reflect ( const FSpirVBlob &InSpirV, EShaderStage::Type InStage, FShaderReflection &OutReflection, FString &OutError ) noexcept;
        };

    } // namespace Internal

} // namespace Compiler

} // namespace LumenEngine