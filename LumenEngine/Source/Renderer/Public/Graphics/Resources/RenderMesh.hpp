/**
 * @file RenderMesh.hpp
 * @brief Definition of the FRenderMesh class, representing a mesh resource for rendering.
 */

#pragma once

#include "Container/Vector.hpp"
#include "Maths/Vertex.hpp"
#include "RHI/RHITypes.hpp"

namespace LumenEngine
{

namespace Renderer
{

    struct LUMEN_ENGINE_API FRenderMesh
    {
        TVector<Maths::FVertex> Vertices;
        TVector<UInt32> Indices;
        RHI::FMeshHandle RenderHandle;
    };

} // namespace Renderer

} // namespace LumenEngine