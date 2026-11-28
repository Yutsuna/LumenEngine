/**
 * @file RenderSurfaceDesc.hpp
 * @brief Backend-neutral description of the native surface a renderer presents to.
 */

#pragma once

#include <RendererExport.hpp>

#include <Core/CoreTypes.hpp>
#include <Execution/RenderFrameTypes.hpp>
#include <String/String.hpp>

namespace Lumen
{

/**
 * @struct FRenderSurfaceDesc
 * @brief Opaque native handles plus a window-system identifier string.
 * @details The identifiers are plain text ("Win32", "X11", "Wayland"); Renderer never includes
 *          AppCore headers or dereferences the handles. Launch maps its own handles to this struct.
 */
struct RENDERER_EXPORT FRenderSurfaceDesc final
{
    FString Source;
    UPtr NativeWindow           = 0U;
    UPtr NativeDisplay          = 0U;
    UPtr NativeWindowAttachment = 0U;
    Bool bRequiresDisplay       = false;
    Bool bPresent               = false;
    FRenderExtent3D InitialExtent{ 1U, 1U, 1U };
};

} // namespace Lumen
