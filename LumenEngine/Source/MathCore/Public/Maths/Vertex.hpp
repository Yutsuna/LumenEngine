/**
 * @file Vertex.hpp
 * @brief Definition of the FVertex struct
 */

#pragma once

#include "Definitions.hpp"
#include "Maths/Vec.hpp"

namespace LumenEngine
{

namespace Maths
{

    /**
     * @struct FVertex
     * @brief Represents a vertex in 3D space, containing position, normal, UV coordinates, and tangent information.
     */
    struct LUMEN_ENGINE_API FVertex final
    {
        Maths::FVec3f Position;
        Maths::FVec3f Normal;
        Maths::FVec2f UV;
        Maths::FVec3f Tangent;

        constexpr FVertex () noexcept = default;
        constexpr FVertex ( const Maths::FVec3f &InPosition, const Maths::FVec3f &InNormal, const Maths::FVec2f &InUV, const Maths::FVec3f &InTangent ) noexcept;

        constexpr LumenEngine::Bool operator==( const FVertex &Other ) const noexcept;
        constexpr LumenEngine::Bool operator!=( const FVertex &Other ) const noexcept;
    };

} // namespace Maths

} // namespace LumenEngine

#include "Inline/Vertex.inl"
