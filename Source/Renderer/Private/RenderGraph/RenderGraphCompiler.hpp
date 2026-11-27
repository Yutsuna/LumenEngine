/**
 * @file RenderGraphCompiler.hpp
 * @brief Internal stages of deterministic CPU render graph compilation.
 */

#pragma once

#include <RenderGraph/CompiledRenderGraph.hpp>

namespace Lumen
{

class FRenderGraphCompiler final
{
public:

    [[nodiscard]] static TExpected<FCompiledRenderGraph, FRenderError> Compile ( FRenderGraph &&InGraph ) noexcept;

private:

    [[nodiscard]] static FRenderExpected BuildResourceDependencies ( FRenderGraph &InGraph ) noexcept;
    [[nodiscard]] static FRenderExpected AddDependency ( FRenderGraph &InGraph, UInt32 InBefore, UInt32 InAfter ) noexcept;
    [[nodiscard]] static FRenderExpected TopologicalSort ( FRenderGraph &InGraph, Bool bActiveOnly ) noexcept;
    [[nodiscard]] static FRenderExpected CullPasses ( FRenderGraph &InGraph ) noexcept;
    [[nodiscard]] static FRenderExpected BuildLifetimes ( FRenderGraph &InGraph ) noexcept;
    [[nodiscard]] static FRenderExpected BuildQueueSegments ( FRenderGraph &InGraph ) noexcept;
    [[nodiscard]] static FRenderExpected
    AddQueueDependency ( FRenderGraph &InGraph, UInt32 InSource, UInt32 InDestination ) noexcept;
    [[nodiscard]] static FRenderExpected BuildTransitions ( FRenderGraph &InGraph ) noexcept;
    [[nodiscard]] static FRenderExpected EmitTransition ( FRenderGraph &InGraph,
                                                          FRenderResource &InResource,
                                                          RHI::ERHIResourceState InBefore,
                                                          RHI::ERHIResourceState InAfter,
                                                          RHI::ERHIQueueType InSourceQueue,
                                                          RHI::ERHIQueueType InDestinationQueue,
                                                          UInt32 InSourcePass,
                                                          UInt32 InDestinationPass ) noexcept;
};

} // namespace Lumen
