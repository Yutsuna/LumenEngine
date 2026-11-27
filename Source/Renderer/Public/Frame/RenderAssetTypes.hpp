/**
 * @file RenderAssetTypes.hpp
 * @brief Value contracts for geometry asset registration.
 */

#pragma once

#include <RendererExport.hpp>

#include <Core/CoreTypes.hpp>

#include <Maths/Vec.hpp>
#include <Memory/SharedPtr.hpp>
#include <Memory/Span.hpp>
#include <RHI/Commands/RHIDrawArguments.hpp>
#include <RHI/Resources/IRHIBuffer.hpp>
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

/**
 * @struct FRenderGeometryAsset
 * @brief Device-owned vertex and index buffers produced from a descriptor.
 */
struct FRenderGeometryAsset final
{
    FRenderAssetId Id;
    TSharedRef<RHI::IRHIBuffer> VertexBuffer;
    TSharedRef<RHI::IRHIBuffer> IndexBuffer;
    UInt32 VertexCount               = 0U;
    UInt32 IndexCount                = 0U;
    RHI::ERHIIndexFormat IndexFormat = RHI::ERHIIndexFormat::Index32;
};

} // namespace Lumen
