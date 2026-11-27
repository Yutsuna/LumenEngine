/**
 * @file RenderFrameContracts.hpp
 * @brief Composition-root inputs for the first opaque image and its readback.
 */

#pragma once

#include <RendererExport.hpp>

#include <Core/CoreTypes.hpp>

#include <String/String.hpp>

#include <RHI/Types/RHIShaderFormat.hpp>

#include <Templates/Vector.hpp>

namespace Lumen
{

/**
 * @struct FRenderShaderSet
 * @brief Portable shader bytes for the opaque forward pass; no backend object is carried.
 * @details Empty bytecode disables the forward pass and keeps the clear-only slice. The bytes are
 *          interpreted by the selected RHI backend through the negotiated ERHIShaderFormat; this
 *          module never translates, compiles or inspects shader language text.
 */
struct RENDERER_EXPORT FRenderShaderSet final
{
    TVector<Byte> VertexBytecode;
    TVector<Byte> PixelBytecode;
    FString VertexEntryPoint     = "Main";
    FString PixelEntryPoint      = "Main";
    RHI::ERHIShaderFormat Format = RHI::ERHIShaderFormat::Null;
    UInt32 InlineConstantBytes   = 128U;
};

/**
 * @struct FRenderFrameReadbackDesc
 * @brief Enables a bounded CPU-visible copy of the rendered target.
 */
struct RENDERER_EXPORT FRenderFrameReadbackDesc final
{
    UInt64 Capacity = 0ULL;
};

} // namespace Lumen
