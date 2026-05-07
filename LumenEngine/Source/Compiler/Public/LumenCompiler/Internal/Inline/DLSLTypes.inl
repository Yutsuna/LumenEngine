/**
 * @file DLSLTypes.inl
 * @brief Inline implementations for DLSLTypes.hpp.
 */

#pragma once

#include "LumenCompiler/Internal/DLSLTypes.hpp"

/**
 * EDLSLNodeType
 */

constexpr const LumenEngine::AnsiChar *LumenEngine::Compiler::EDLSLNodeType::ToString ( Type InType ) noexcept
{
    switch ( InType )
    {
    case Type::String:
        return "String";
    case Type::Number:
        return "Number";
    case Type::Vector:
        return "Vector";
    case Type::Boolean:
        return "Boolean";
    case Type::Identifier:
        return "Identifier";
    case Type::List:
        return "List";
    case Type::Object:
        return "Object";
    default:
        return "Unknown";
    }
}

/**
 * FDLSLNode
 */

inline LumenEngine::FStringView LumenEngine::Compiler::FDLSLNode::GetString () const noexcept
{
    return { StringRaw.Data, StringRaw.Size };
}
