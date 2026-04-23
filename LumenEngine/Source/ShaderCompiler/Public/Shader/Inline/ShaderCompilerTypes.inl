/**
 * @file ShaderCompilerTypes.inl
 * @brief Inline implementations of shader compiler types.
 */

#pragma once

#include "Shader/ShaderCompilerTypes.hpp"

/**
 * EShaderStage
 */

constexpr const LumenEngine::AnsiChar *LumenEngine::EShaderStage::ToString ( const Type InStage ) noexcept
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

constexpr const LumenEngine::AnsiChar *LumenEngine::EShaderStage::ToGlslExtension ( const Type InStage ) noexcept
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

/**
 * EShaderCompilerError
 */

constexpr const LumenEngine::AnsiChar *LumenEngine::EShaderCompilerError::ToString ( const Type InError ) noexcept
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