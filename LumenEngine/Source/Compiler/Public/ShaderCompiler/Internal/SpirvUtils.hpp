/**
 * @file SpirvUtils.hpp
 * @brief Utilities for SPIR-V validation and debugging.
 */

#pragma once

#include "Container/String.hpp"
#include "ShaderCompiler/ShaderCompilerTypes.hpp"

namespace LumenEngine
{

namespace Compiler
{

    namespace Internal
    {

        /**
         * @class FSpirvUtils
         * @brief Encapsulates spirv-tools library logic.
         */
        class FSpirvUtils final
        {
        public:

            /**
             * @brief Disassemble a SPIR-V blob to human-readable assembly text.
             * @param InSpirV SPIR-V bytecode.
             * @return SPIR-V assembly text.
             */
            [[nodiscard]] static FString Disassemble ( const FSpirVBlob &InSpirV ) noexcept;

            /**
             * @brief Validate SPIR-V blob using Vulkan rules.
             * @param InSpirV SPIR-V bytecode.
             * @return Empty string on success, error log on failure.
             */
            [[nodiscard]] static FString Validate ( const FSpirVBlob &InSpirV ) noexcept;
        };

    } // namespace Internal

} // namespace Compiler

} // namespace LumenEngine