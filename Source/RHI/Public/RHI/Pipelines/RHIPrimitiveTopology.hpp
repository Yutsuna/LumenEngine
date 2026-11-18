/**
 * @file RHIPrimitiveTopology.hpp
 * @brief Defines primitive assembly modes.
 */

#pragma once

#include <Core/CoreTypes.hpp>

namespace Lumen
{

namespace RHI
{

    /**
     * @enum ERHIPrimitiveTopology
     * @brief Portable primitive topologies.
     */
    enum class ERHIPrimitiveTopology : UInt8
    {
        PointList,
        LineList,
        LineStrip,
        TriangleList,
        TriangleStrip,
        PatchList
    };

} // namespace RHI

} // namespace Lumen
