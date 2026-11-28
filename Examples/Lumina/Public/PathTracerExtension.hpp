/**
 * @file PathTracerExtension.hpp
 * @brief Lumina's portable seven-pass path-tracing frame extension.
 */
#pragma once

#include <Execution/RenderFrameExtension.hpp>
#include <Maths/Vec.hpp>
#include <RenderGraph/RenderGraphBuilder.hpp>
#include <RenderGraph/RenderGraphContext.hpp>
#include <RHI/Commands/RHIRenderingDesc.hpp>
#include <RHI/Resources/RHIResourceUsage.hpp>
#include <RHI/Types/RHIFormat.hpp>
#include <String/String.hpp>
#include <Templates/Array.hpp>
#include <Templates/Vector.hpp>

namespace Lumen
{

namespace RHI
{
    class IRHIBindingLayout;
    class IRHIBindingSet;
    class IRHIPipeline;
    class IRHISampler;
    class IRHIShader;
} // namespace RHI

/** @class FLuminaPathTracerExtension @brief Owns accumulation, bloom, display pipelines and their commit-time state. */
class FLuminaPathTracerExtension final : public IRenderFrameExtension
{
public:
    explicit FLuminaPathTracerExtension ( Bool bSoftwareAdapter ) noexcept;

    [[nodiscard]] RHI::FRHIExpected Initialize ( RHI::IRHIDevice &InDevice,
                                                 const RHI::FRHIExtent3D &InExtent ) noexcept override;
    [[nodiscard]] RHI::FRHIExpected Resize ( const RHI::FRHIExtent3D &InExtent ) noexcept override;
    [[nodiscard]] RHI::FRHIExpected Record ( const FRenderFrameExtensionContext &InContext ) noexcept override;
    void Commit ( UInt64 InTimelineValue, FRenderFrameStatus &InOutStatus ) noexcept override;

private:
    struct FTransitionRecord
    {
        USize TransitionIndex = 0ULL;
        UInt32 PassOrdinal    = 0U;
        ERenderTransitionRecordPoint Point = ERenderTransitionRecordPoint::BeforePass;
    };

    [[nodiscard]] RHI::FRHIExtent3D ScaleExtent ( const RHI::FRHIExtent3D &InExtent ) const noexcept;
    [[nodiscard]] RHI::FRHIExpected CreateFrameResources ( const RHI::FRHIExtent3D &InExtent ) noexcept;
    [[nodiscard]] TExpected<TSharedRef<RHI::IRHIPipeline>, RHI::FRHIError>
    CreatePipeline ( const FString &InFragmentSource,
                     UInt32 InInlineConstantBytes,
                     TSharedRef<RHI::IRHIBindingLayout> InBindingLayout,
                     RHI::ERHIFormat InTargetFormat ) noexcept;
    [[nodiscard]] RHI::FRHIExpected RecordTransitions ( const FRenderGraphContext &InGraph,
                                                        RHI::IRHICommandList &InCommandList,
                                                        UInt32 InPassOrdinal,
                                                        ERenderTransitionRecordPoint InPoint ) noexcept;
    [[nodiscard]] RHI::FRHIExpected RecordDraw ( const FRenderGraphContext &InGraph,
                                                 RHI::IRHICommandList &InCommandList,
                                                 UInt32 InPassOrdinal,
                                                 TSharedRef<RHI::IRHIPipeline> InPipeline,
                                                 TSharedRef<RHI::IRHIBindingSet> InBindingSet,
                                                 TSharedRef<RHI::IRHITextureView> InTargetView,
                                                 const RHI::FRHIExtent3D &InTargetExtent,
                                                 TSpan<const Byte> InConstants ) noexcept;

private:
    RHI::IRHIDevice *Device = nullptr;
    Bool bSoftwareAdapter   = false;
    FString VertexSource;
    FString PathTraceSource;
    FString BloomExtractSource;
    FString BloomBlurSource;
    FString DisplaySource;
    TOptional<TSharedRef<RHI::IRHIShader>> VertexShader;
    TSharedPtr<RHI::IRHISampler> NearestSampler;
    TSharedPtr<RHI::IRHISampler> LinearSampler;
    TOptional<TSharedRef<RHI::IRHIBindingLayout>> OneTextureLayout;
    TOptional<TSharedRef<RHI::IRHIBindingLayout>> TwoTextureLayout;
    TOptional<TSharedRef<RHI::IRHIPipeline>> PathTracePipeline;
    TOptional<TSharedRef<RHI::IRHIPipeline>> BloomExtractPipeline;
    TOptional<TSharedRef<RHI::IRHIPipeline>> BloomBlurPipeline;
    TOptional<TSharedRef<RHI::IRHIPipeline>> DisplayPipeline;

    TVector<TSharedRef<RHI::IRHITexture>> AccumulationTextures;
    TVector<TSharedRef<RHI::IRHITextureView>> AccumulationViews;
    TVector<TSharedRef<RHI::IRHITexture>> BloomTextures;
    TVector<TSharedRef<RHI::IRHITextureView>> BloomViews;
    TVector<TSharedRef<RHI::IRHIBindingSet>> PathTraceBindings;
    TVector<TSharedRef<RHI::IRHIBindingSet>> BloomExtractBindings;
    TVector<TSharedRef<RHI::IRHIBindingSet>> DisplayBindings;
    TOptional<TSharedRef<RHI::IRHIBindingSet>> BlurHorizontalBinding;
    TOptional<TSharedRef<RHI::IRHIBindingSet>> BlurVerticalBinding;

    TOptional<FRenderGraphBuilder> GraphBuilder;
    RHI::FRHIRenderingDesc RenderingDesc;
    TVector<FTransitionRecord> TransitionRecords;
    RHI::FRHIExtent3D AccumulationExtent{ 1U, 1U, 1U };
    RHI::FRHIExtent3D BloomExtent{ 1U, 1U, 1U };
    TArray<RHI::ERHIResourceState, 2U> AccumulationStates{};
    TArray<RHI::ERHIResourceState, 2U> BloomStates{};
    TArray<RHI::ERHIResourceState, 2U> PendingAccumulationStates{};
    TArray<RHI::ERHIResourceState, 2U> PendingBloomStates{};
    Maths::FVec3f LastCameraPosition;
    UInt64 SampleCount       = 0ULL;
    UInt32 PingPong          = 0U;
    UInt32 PendingWriteIndex = 0U;
    Bool bHasCamera          = false;
};

} // namespace Lumen
