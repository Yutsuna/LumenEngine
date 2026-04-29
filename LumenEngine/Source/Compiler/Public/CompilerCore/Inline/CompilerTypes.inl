/**
 * @file CompilerTypes.inl
 * @brief Inline implementations for CompilerTypes.hpp.
 */

#pragma once

#include "CompilerCore/CompilerTypes.hpp"

constexpr const LumenEngine::AnsiChar *LumenEngine::Compiler::ECompilerError::ToString ( const Type InError ) noexcept
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
    case BackendInitFailed:
        return "BackendInitFailed";
    case UnsupportedProfile:
        return "UnsupportedProfile";
    case SyntaxError:
        return "SyntaxError";
    case SerializationFailed:
        return "SerializationFailed";
    default:
        return "Unknown";
    }
}
