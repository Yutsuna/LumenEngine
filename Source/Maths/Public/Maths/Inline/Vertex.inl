///
/// @file Vertex.inl
/// @brief Implementation of the FVertex struct
///

#pragma once

#include <Maths/Vertex.hpp>

constexpr Lumen::Maths::FVertex::FVertex ( const FVec3f &InPosition,
                                           const FVec3f &InNormal,
                                           const FVec2f &InUV,
                                           const FVec3f &InTangent ) noexcept
    : Position( InPosition ), Normal( InNormal ), UV( InUV ), Tangent( InTangent )
{
    ///
}

constexpr Lumen::Maths::FVertex::FVertex ( const FVec3f &InPosition ) noexcept : Position( InPosition )
{
    ///
}

constexpr Lumen::Bool Lumen::Maths::FVertex::operator==( const FVertex &Other ) const noexcept
{
    return Position == Other.Position and Normal == Other.Normal and UV == Other.UV and Tangent == Other.Tangent;
}

constexpr Lumen::Bool Lumen::Maths::FVertex::operator!=( const FVertex &Other ) const noexcept
{
    return !( *this == Other );
}
