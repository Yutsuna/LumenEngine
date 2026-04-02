/**
 * @file RenderMesh.hpp
 * @brief Definition of the FRenderMesh class, representing a mesh resource for rendering.
 */

#pragma once

#include "Container/Vector.hpp"
#include "Graphics/RenderTypes.hpp"
#include "Maths/Vertex.hpp"

namespace LumenEngine
{

namespace Renderer
{

    struct LUMEN_ENGINE_API FRenderMesh
    {
        TVector<Maths::FVertex> Vertices;
        TVector<UInt32> Indices;
        FMeshHandle RenderHandle = InvalidHandle;
    };

} // namespace Renderer

} // namespace LumenEngine
