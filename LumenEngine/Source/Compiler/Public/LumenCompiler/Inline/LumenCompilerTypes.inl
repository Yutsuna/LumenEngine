/**
 * @file LumenCompilerTypes.inl
 * @brief Inline implementations for LumenCompilerTypes.hpp.
 */

#pragma once

#include "LumenCompiler/LumenCompilerTypes.hpp"

/**
 * EAssetType
 */

constexpr const LumenEngine::AnsiChar *LumenEngine::Compiler::EAssetType::ToString ( Type InAssetType ) noexcept
{
    switch ( InAssetType )
    {
    case Type::Unknown:
        return "Unknown";
    case Type::Mesh:
        return "Mesh";
    case Type::Material:
        return "Material";
    case Type::Shader:
        return "Shader";
    default:
        return "Unknown";
    }
}

/**
 * ELumenCompilerError
 */

constexpr const LumenEngine::AnsiChar *LumenEngine::Compiler::ELumenCompilerError::ToString ( Type InError ) noexcept
{
    switch ( InError )
    {
    case Type::None:
        return "None";
    case Type::FileNotFound:
        return "FileNotFound";
    case Type::ReadFailed:
        return "ReadFailed";
    case Type::CompilationFailed:
        return "CompilationFailed";
    case Type::ParseFailed:
        return "ParseFailed";
    case Type::SerializationFailed:
        return "SerializationFailed";
    case Type::InvalidBlockType:
        return "InvalidBlockType";
    case Type::BlockNotFound:
        return "BlockNotFound";
    case Type::AllocationFailed:
        return "AllocationFailed";
    default:
        return "Unknown";
    }
}

/**
 * FCompiledLumenAsset
 */

inline LumenEngine::UInt64 LumenEngine::Compiler::FCompiledLumenAsset::GetByteSize () const noexcept
{
    return BinaryBlob.size();
}

inline LumenEngine::Bool LumenEngine::Compiler::FCompiledLumenAsset::IsValid () const noexcept
{
    return not BinaryBlob.empty();
}

/**
 * FLumenCompileResult
 */

inline LumenEngine::Bool LumenEngine::Compiler::FLumenCompileResult::IsSuccess () const noexcept
{
    return Asset.has_value() and Asset->IsValid() and Error == ELumenCompilerError::None;
}

inline LumenEngine::Compiler::FLumenCompileResult LumenEngine::Compiler::FLumenCompileResult::Success ( FCompiledLumenAsset &&InAsset ) noexcept
{
    return { .Asset = std::move( InAsset ), .Error = ELumenCompilerError::None, .ErrorLog = {} };
}

inline LumenEngine::Compiler::FLumenCompileResult LumenEngine::Compiler::FLumenCompileResult::Failure ( ELumenCompilerError::Type InError, FString InLog ) noexcept
{
    return { .Asset = std::nullopt, .Error = InError, .ErrorLog = std::move( InLog ) };
}
