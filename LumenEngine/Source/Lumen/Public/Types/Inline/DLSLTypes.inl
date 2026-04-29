/**
 * @file DLSLTypes.inl
 * @brief Inline implementations for the DLSL types.
 */

#pragma once

#include "Types/DLSLTypes.hpp"

constexpr const LumenEngine::AnsiChar *LumenEngine::Lumen::EDLSLNodeType::ToString ( Type InType ) noexcept
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