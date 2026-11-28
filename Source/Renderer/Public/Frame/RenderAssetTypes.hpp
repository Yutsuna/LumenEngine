/**
 * @file RenderAssetTypes.hpp
 * @brief Value contracts for geometry asset registration.
 */

#pragma once

#include <RendererExport.hpp>

#include <Core/CoreTypes.hpp>

#include <Maths/Vec.hpp>
#include <Memory/Span.hpp>
#include <Rendering/RenderIds.hpp>

namespace Lumen
{

/**
 * @enum ERenderAssetError
 * @brief Failures reported while registering a geometry asset.
 */
enum class ERenderAssetError : UInt8
{
    InvalidDesc,
    CapacityExceeded,
    RHIError,
};

/**
 * @struct FRenderGeometryAssetDesc
 * @brief CPU-side geometry data borrowed for the duration of Register.
 */
struct FRenderGeometryAssetDesc final
{
    FRenderAssetId Id;
    TSpan<const Maths::FVec3f> Positions;
    TSpan<const UInt32> Indices;
};

} // namespace Lumen
