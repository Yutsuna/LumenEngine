/**
 * @file RenderResource.hpp
 * @brief Base struct | class for render resources, providing an interface for initialization and cleanup.
 */

#pragma once

#include "CoreTypes.hpp"

namespace LumenEngine
{

/**
 * @struct FRenderPacket
 * @brief Data packet sent from the Game Thread to the Render Thread every frame.
 */
struct FRenderPacket
{
    Float32 ClearColor[4] = { 0.F, 0.F, 0.F, 1.F };
};

} // namespace LumenEngine
