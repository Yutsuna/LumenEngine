/**
 * @file PathTracerExtension.hpp
 * @brief Lumina's portable seven-pass path-tracing frame extension.
 */
#pragma once

#include <Execution/RenderFeature.hpp>
#include <Execution/RenderFrameBuilder.hpp>
#include <Maths/Vec.hpp>
#include <String/String.hpp>
#include <Templates/Array.hpp>
#include <Templates/Vector.hpp>

namespace Lumen
{

/** @class FLuminaPathTracerExtension @brief Owns accumulation, bloom and display pipelines. */
class FLuminaPathTracerExtension final : public IRenderFeature
{
public:

    FLuminaPathTracerExtension () noexcept = default;

    [[nodiscard]] FRenderExtensionExpected Initialize ( IRenderFeatureHost &InHost,
                                                        const FRenderExtent3D &InExtent ) noexcept override;
    [[nodiscard]] FRenderExtensionExpected Resize ( const FRenderExtent3D &InExtent ) noexcept override;
    [[nodiscard]] FRenderExtensionExpected Record ( const FRenderFrameExtensionContext &InContext ) noexcept override;
    void Commit ( UInt64 InTimelineValue, FRenderFrameStatus &InOutStatus ) noexcept override;

private:

    [[nodiscard]] FRenderExtent3D ScaleExtent ( const FRenderExtent3D &InExtent ) const noexcept;
    [[nodiscard]] FRenderExtensionExpected CreateFrameResources ( const FRenderExtent3D &InExtent ) noexcept;
    [[nodiscard]] TExpected<FRenderPipelineRef, FRenderExtensionError> CreatePipeline ( IRenderFeatureHost &InHost,
                                                                                        const FString &InFragmentSource,
                                                                                        USize InInlineConstantBytes,
                                                                                        FRenderBindingLayoutRef InBindingLayout,
                                                                                        ERenderFormat InTargetFormat ) noexcept;

private:

    IRenderFeatureHost *Host = nullptr;
    Bool bSoftwareAdapter    = false;
    FString VertexSource;
    FString PathTraceSource;
    FString BloomExtractSource;
    FString BloomBlurSource;
    FString DisplaySource;
    FRenderShaderRef VertexShader;
    FRenderSamplerRef NearestSampler;
    FRenderSamplerRef LinearSampler;
    FRenderBindingLayoutRef OneTextureLayout;
    FRenderBindingLayoutRef TwoTextureLayout;
    FRenderPipelineRef PathTracePipeline;
    FRenderPipelineRef BloomExtractPipeline;
    FRenderPipelineRef BloomBlurPipeline;
    FRenderPipelineRef DisplayPipeline;

    TVector<FRenderTextureRef> AccumulationTextures;
    TVector<FRenderTextureViewRef> AccumulationViews;
    TVector<FRenderTextureRef> BloomTextures;
    TVector<FRenderTextureViewRef> BloomViews;
    TVector<FRenderBindingSetRef> PathTraceBindings;
    TVector<FRenderBindingSetRef> BloomExtractBindings;
    TVector<FRenderBindingSetRef> DisplayBindings;
    FRenderBindingSetRef BlurHorizontalBinding;
    FRenderBindingSetRef BlurVerticalBinding;

    TUniquePtr<IRenderFrameBuilder> FrameBuilder;
    FRenderExtent3D AccumulationExtent{ 1U, 1U, 1U };
    FRenderExtent3D BloomExtent{ 1U, 1U, 1U };
    Maths::FVec3f LastCameraPosition;
    Maths::FVec3f LastCameraForward;
    Float32 LastTanHalfFov = 0.0F;
    UInt64 SampleCount     = 0ULL;
    UInt32 PingPong        = 0U;
    Bool bHasCamera        = false;
};

} // namespace Lumen
