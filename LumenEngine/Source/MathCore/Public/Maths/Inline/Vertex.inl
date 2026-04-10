/**
 * @file Vertex.inl
 * @brief Implementation of the FVertex struct
 */

#pragma once

#include "Maths/Vertex.hpp"

constexpr LumenEngine::Maths::FVertex::FVertex ( const FVec3f &InPosition, const FVec3f &InNormal, const FVec2f &InUV, const FVec3f &InTangent ) noexcept
    : Position( InPosition ), Normal( InNormal ), UV( InUV ), Tangent( InTangent )
{
    //
}

constexpr LumenEngine::Maths::FVertex::FVertex ( const FVec3f &InPosition ) noexcept : Position( InPosition )
{
    //
}

constexpr LumenEngine::Bool LumenEngine::Maths::FVertex::operator==( const FVertex &Other ) const noexcept
{
    return Position == Other.Position && Normal == Other.Normal && UV == Other.UV && Tangent == Other.Tangent;
}

constexpr LumenEngine::Bool LumenEngine::Maths::FVertex::operator!=( const FVertex &Other ) const noexcept
{
    return !( *this == Other );
}
