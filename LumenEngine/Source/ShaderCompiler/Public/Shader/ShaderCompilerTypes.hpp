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

    [[nodiscard]] constexpr const AnsiChar *ToString ( const Type InStage ) noexcept
    {
        switch ( InStage )
        {
        case Vertex:
            return "Vertex";
        case Fragment:
            return "Fragment";
        case Geometry:
            return "Geometry";
        case Compute:
            return "Compute";
        case TessellationControl:
            return "TessellationControl";
        case TessellationEvaluation:
            return "TessellationEvaluation";
        default:
            return "Unknown";
        }
    }

    [[nodiscard]] constexpr const AnsiChar *ToGlslExtension ( const Type InStage ) noexcept
    {
        switch ( InStage )
        {
        case Vertex:
            return ".vert";
        case Fragment:
            return ".frag";
        case Geometry:
            return ".geom";
        case Compute:
            return ".comp";
        case TessellationControl:
            return ".tesc";
        case TessellationEvaluation:
            return ".tese";
        default:
            return ".glsl";
        }
    }

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

    [[nodiscard]] constexpr const AnsiChar *ToString ( const Type InError ) noexcept
    {
        switch ( InError )
        {
        case None:
            return "None";
        case FileNotFound:
            return "FileNotFound";
        case ReadFailed:
            return "ReadFailed";
        case CompilationFailed:
            return "CompilationFailed";
        case OptimizationFailed:
            return "OptimizationFailed";
        case CacheWriteFailed:
            return "CacheWriteFailed";
        case CacheReadFailed:
            return "CacheReadFailed";
        case InvalidStage:
            return "InvalidStage";
        case InvalidSource:
            return "InvalidSource";
        case ReflectionFailed:
            return "ReflectionFailed";
        default:
            return "UnknownError";
        }
    }

} // namespace EShaderCompilerError

} // namespace LumenEngine
