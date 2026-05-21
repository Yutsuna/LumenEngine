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

#include "Logging/LoggingCategory.hpp"

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

        /** @brief Prepares the frame and binds uniform data */
        virtual Bool BeginFrame ( const FGlobalUniformData &InUniforms ) = 0;
        virtual void EndFrame ()                                         = 0;

        virtual void DestroyPipeline ( FPipelineHandle InHandle ) = 0;
        virtual void DestroyMesh ( FMeshHandle InHandle )         = 0;
        virtual void DestroyTexture ( FTextureHandle InHandle )   = 0;

    public:

        /** @brief Returns the command list for the current frame */
        [[nodiscard]] virtual IRHICommandList &GetCommandList () noexcept = 0;

    public:

        virtual FPipelineHandle CreatePipeline ( const FGraphicsPipelineDesc &InDescription )                         = 0;
        virtual FMeshHandle CreateMesh ( const TVector<Maths::FVertex> &InVertices, const TVector<UInt32> &InIndices ) = 0;

        /** @brief Initializes GPU-driven resources with the provided compute shader bytecode */
        virtual void InitializeGpuDrivenResources ( const FShaderByteCode &InComputeCode ) = 0;
    };

    extern LUMEN_ENGINE_API const FLogCategory LogRHI;

} // namespace RHI

} // namespace LumenEngine
