/**
 * @file ShaderCompilerTypes.hpp
 * @brief Declaration of shader compiler types.
 */

#pragma once

#include "CoreTypes.hpp"

namespace LumenEngine
{

namespace EShaderStage
{

    enum Type : UInt8
    {
        Vertex,
        Fragment,
        Geometry,
        Compute,
        TessellationControl,
        TessellationEvaluation,
        Count
    };

    /**
     * @brief Converts a shader stage type to its string representation.
     * @param InStage The shader stage type to convert.
     * @return A string representation of the shader stage type.
     */
    [[nodiscard]] constexpr const AnsiChar *ToString ( const Type InStage ) noexcept;

    /**
     * @brief Converts a shader stage type to its corresponding GLSL file extension.
     * @param InStage The shader stage type to convert.
     * @return A string representing the GLSL file extension for the shader stage type.
     */
    [[nodiscard]] constexpr const AnsiChar *ToGlslExtension ( const Type InStage ) noexcept;

} // namespace EShaderStage

enum class EShaderOptimizationLevel : UInt8
{
    /** -O0 debug only */
    None,

    /** -Os minimize SPIR-V size */
    Size,

    /** -0 full performance optimization (Default) */
    Performance
};

enum class EGlslProfile : UInt8
{
    Core,
    Compatibility,
    Es,
    Vulkan
};

namespace EShaderCompilerError
{

    enum Type : UInt8
    {
        None               = 0,
        FileNotFound       = 1,
        ReadFailed         = 2,
        CompilationFailed  = 3,
        OptimizationFailed = 4,
        CacheWriteFailed   = 5,
        CacheReadFailed    = 6,
        InvalidStage       = 7,
        InvalidSource      = 8,
        ReflectionFailed   = 9,
    };

    /**
     * @brief Converts a shader compiler error type to its string representation.
     * @param InError The shader compiler error type to convert.
     * @return A string representation of the shader compiler error type.
     */
    [[nodiscard]] constexpr const AnsiChar *ToString ( const Type InError ) noexcept;

} // namespace EShaderCompilerError

} // namespace LumenEngine

#include "Inline/ShaderCompilerTypes.inl"