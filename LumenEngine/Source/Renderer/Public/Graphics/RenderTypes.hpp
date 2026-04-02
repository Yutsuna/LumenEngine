/**
 * @file RenderTypes.hpp
 * @brief Common types and definitions for the rendering system.
 */

#pragma once

#include "CoreTypes.hpp"

namespace LumenEngine
{

namespace Renderer
{

    using FShaderHandle  = UInt32;
    using FTextureHandle = UInt32;
    using FMeshHandle    = UInt32;

    static constexpr UInt32 InvalidHandle = 0xFFFFFFFF;

} // namespace Renderer

} // namespace LumenEngine
