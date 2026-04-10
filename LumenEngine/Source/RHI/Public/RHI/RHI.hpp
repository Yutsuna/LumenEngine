/**
 * @file RHI.hpp
 * @brief Abstract Render Hardware Interface
 */

#pragma once

#include "CoreTypes.hpp"

#include "Container/SharedPtr.hpp"
#include "Container/String.hpp"
#include "Container/Vector.hpp"

#include "Maths/Vertex.hpp"
#include "RHI/RHITypes.hpp"

namespace LumenEngine
{

class FGenericWindow;

namespace RHI
{

    class IRHICommandList;

    class LUMEN_ENGINE_API IRHI
    {
    public:

        virtual ~IRHI () noexcept = default;

    public:

        virtual void Initialize ( const TSharedPtr<FGenericWindow> &InWindow ) = 0;
        virtual void Shutdown ()                                               = 0;
        virtual void WaitIdle () const noexcept                                = 0;

        virtual Bool BeginFrame () = 0;
        virtual void EndFrame ()   = 0;

    public:

        /** @brief Returns the command list for the current frame */
        [[nodiscard]] virtual IRHICommandList &GetCommandList () noexcept = 0;

        virtual FPipelineHandle CreatePipeline ( const FString &InVertexPath, const FString &InFragmentPath )          = 0;
        virtual FMeshHandle CreateMesh ( const TVector<Maths::FVertex> &InVertices, const TVector<UInt32> &InIndices ) = 0;
    };

} // namespace RHI

} // namespace LumenEngine