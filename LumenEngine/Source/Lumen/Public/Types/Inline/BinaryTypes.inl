/**
 * @file BinaryTypes.inl
 * @brief Inline implementations for the binary types.
 */

#pragma once

#include "Types/BinaryTypes.hpp"

constexpr const LumenEngine::AnsiChar *LumenEngine::Lumen::EAssetType::ToString ( Type AssetType ) noexcept
{
    switch ( AssetType )
    {
    case Type::Unknown:
        return "Unknown";
    case Type::Mesh:
        return "Mesh";
    case Type::Material:
        return "Material";
    default:
        return "Unknown";
    }
}