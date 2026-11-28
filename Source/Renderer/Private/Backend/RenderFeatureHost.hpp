/**
 * @file RenderFeatureHost.hpp
 * @brief Private implementation of the neutral feature host, frame builder and pass encoder.
 */

#pragma once

#include <Execution/RenderFeature.hpp>
#include <Execution/RenderFrameBuilder.hpp>

#include <Memory/SharedPtr.hpp>
#include <RHI/Commands/IRHICommandList.hpp>
#include <RHI/Commands/RHIRenderingDesc.hpp>
#include <RHI/Device/IRHIDevice.hpp>
#include <RenderGraph/CompiledRenderGraph.hpp>
#include <RenderGraph/RenderGraphBuilder.hpp>
#include <RenderGraph/RenderGraphContext.hpp>
#include <Templates/Optional.hpp>
#include <Templates/Pair.hpp>
#include <Templates/Vector.hpp>

namespace Lumen
{

/**
 * @struct FRenderTextureObject
 * @brief Opaque texture plus the private committed barrier state.
 */
struct FRenderTextureObject final : IRenderOpaqueObject
{
    explicit FRenderTextureObject ( TSharedRef<RHI::IRHITexture> InTexture,
                                    RHI::ERHIResourceState InState = RHI::ERHIResourceState::Common ) noexcept;

    TSharedRef<RHI::IRHITexture> Texture;
    RHI::ERHIResourceState CommittedState;
};

/** @brief Opaque texture view plus its owning texture. */
struct FRenderTextureViewObject final : IRenderOpaqueObject
{
    FRenderTextureViewObject ( TSharedRef<RHI::IRHITextureView> InView, TSharedPtr<FRenderTextureObject> InTexture ) noexcept;

    TSharedRef<RHI::IRHITextureView> View;
    TSharedPtr<FRenderTextureObject> Texture;
};

/** @brief Opaque shader object. */
struct FRenderShaderObject final : IRenderOpaqueObject
{
    explicit FRenderShaderObject ( TSharedRef<RHI::IRHIShader> InShader ) noexcept;

    TSharedRef<RHI::IRHIShader> Shader;
};

/** @brief Opaque sampler object. */
struct FRenderSamplerObject final : IRenderOpaqueObject
{
    explicit FRenderSamplerObject ( TSharedRef<RHI::IRHISampler> InSampler ) noexcept;

    TSharedRef<RHI::IRHISampler> Sampler;
};

/** @brief Opaque binding layout object. */
struct FRenderBindingLayoutObject final : IRenderOpaqueObject
{
    explicit FRenderBindingLayoutObject ( TSharedRef<RHI::IRHIBindingLayout> InLayout ) noexcept;

    TSharedRef<RHI::IRHIBindingLayout> Layout;
};

/** @brief Opaque pipeline layout object. */
struct FRenderPipelineLayoutObject final : IRenderOpaqueObject
{
    explicit FRenderPipelineLayoutObject ( TSharedRef<RHI::IRHIPipelineLayout> InLayout ) noexcept;

    TSharedRef<RHI::IRHIPipelineLayout> Layout;
};

/** @brief Opaque graphics pipeline object. */
struct FRenderPipelineObject final : IRenderOpaqueObject
{
    explicit FRenderPipelineObject ( TSharedRef<RHI::IRHIPipeline> InPipeline ) noexcept;

    TSharedRef<RHI::IRHIPipeline> Pipeline;
};

/** @brief Opaque binding set object. */
struct FRenderBindingSetObject final : IRenderOpaqueObject
{
    explicit FRenderBindingSetObject ( TSharedRef<RHI::IRHIBindingSet> InSet ) noexcept;

    TSharedRef<RHI::IRHIBindingSet> Set;
};

/** @brief One deferred state update applied when the frame is committed. */
struct FRenderPendingState final
{
    FRenderPendingState ( TSharedPtr<FRenderTextureObject> InTexture, RHI::ERHIResourceState InState ) noexcept;

    TSharedPtr<FRenderTextureObject> Texture;
    RHI::ERHIResourceState State;
};

/**
 * @class FRenderFeatureHost
 * @brief Adapts neutral feature factories to the private RHI device.
 */
class FRenderFeatureHost final : public IRenderFeatureHost
{
public:

    FRenderFeatureHost ( TSharedRef<RHI::IRHIDevice> InDevice, Bool bInSoftwareAdapter, FString InDeviceName ) noexcept;

    [[nodiscard]] const FRenderHostCapabilities &GetCapabilities () const noexcept override;

    [[nodiscard]] TRenderExtensionExpected<FRenderShaderRef> CreateShader ( const FRenderShaderDesc &InDesc ) noexcept override;
    [[nodiscard]] TRenderExtensionExpected<FRenderSamplerRef>
    CreateSampler ( const FRenderSamplerDesc &InDesc ) noexcept override;
    [[nodiscard]] TRenderExtensionExpected<FRenderBindingLayoutRef>
    CreateBindingLayout ( const FRenderBindingLayoutDesc &InDesc ) noexcept override;
    [[nodiscard]] TRenderExtensionExpected<FRenderPipelineLayoutRef>
    CreatePipelineLayout ( const FRenderPipelineLayoutDesc &InDesc ) noexcept override;
    [[nodiscard]] TRenderExtensionExpected<FRenderPipelineRef>
    CreateGraphicsPipeline ( const FRenderGraphicsPipelineDesc &InDesc ) noexcept override;
    [[nodiscard]] TRenderExtensionExpected<FRenderTextureRef>
    CreateTexture ( const FRenderTextureDesc &InDesc ) noexcept override;
    [[nodiscard]] TRenderExtensionExpected<FRenderTextureViewRef>
    CreateTextureView ( const FRenderTextureRef &InTexture ) noexcept override;
    [[nodiscard]] TRenderExtensionExpected<FRenderBindingSetRef>
    CreateBindingSet ( const FRenderBindingLayoutRef &InLayout ) noexcept override;
    [[nodiscard]] FRenderExtensionExpected WriteBindingSet ( const FRenderBindingSetRef &InSet,
                                                             TSpan<const FRenderBindingWrite> InWrites ) noexcept override;
    [[nodiscard]] TUniquePtr<IRenderFrameBuilder>
    CreateFrameBuilder ( const FRenderFrameCapacities &InCapacities ) noexcept override;

    /** @brief Applies deferred barrier states after the frame was accepted. */
    void CommitStates () noexcept;

    /** @brief Drops deferred states from a frame that was rejected before submission. */
    void DiscardStates () noexcept;

    /** @brief Wraps an internally owned target into opaque feature references. */
    [[nodiscard]] TPair<FRenderTextureRef, FRenderTextureViewRef>
    WrapExternalTexture ( TSharedRef<RHI::IRHITexture> InTexture, TSharedRef<RHI::IRHITextureView> InView ) noexcept;

    /** @brief Queues a state update for CommitStates. */
    void QueueState ( TSharedPtr<FRenderTextureObject> InTexture, RHI::ERHIResourceState InState ) noexcept;

    [[nodiscard]] RHI::IRHIDevice &GetDevice () const noexcept;

private:

    TSharedRef<RHI::IRHIDevice> Device;
    FRenderHostCapabilities Capabilities;
    TVector<FRenderPendingState> PendingStates;
};

/**
 * @class FRenderPassEncoderAdapter
 * @brief Neutral pass encoder over the private command list.
 */
class FRenderPassEncoderAdapter final : public IRenderPassEncoder
{
public:

    explicit FRenderPassEncoderAdapter ( RHI::IRHICommandList &InCommandList ) noexcept;

    [[nodiscard]] RHI::IRHICommandList &GetCommandList () const noexcept;

    [[nodiscard]] FRenderExtensionExpected SetPipeline ( const FRenderPipelineRef &InPipeline ) noexcept override;
    [[nodiscard]] FRenderExtensionExpected BindSet ( UInt32 InSlot, const FRenderBindingSetRef &InSet ) noexcept override;
    [[nodiscard]] FRenderExtensionExpected
    SetConstants ( ERenderShaderStage InStage, UInt32 InOffset, TSpan<const Byte> InConstants ) noexcept override;
    [[nodiscard]] FRenderExtensionExpected Draw ( UInt32 InVertexCount ) noexcept override;

private:

    RHI::IRHICommandList &CommandList;
};

/**
 * @class FRenderFrameBuilderAdapter
 * @brief Declarative frame graph over the private RHI render graph.
 */
class FRenderFrameBuilderAdapter final : public IRenderFrameBuilder
{
public:

    FRenderFrameBuilderAdapter ( FRenderFeatureHost &InHost, const FRenderFrameCapacities &InCapacities ) noexcept;

    void Reset () noexcept override;
    [[nodiscard]] TRenderExtensionExpected<FRenderFrameResourceHandle>
    Import ( const FRenderTextureRef &InTexture ) noexcept override;
    [[nodiscard]] FRenderExtensionExpected SetTarget ( const FRenderTextureRef &InTexture ) noexcept override;
    [[nodiscard]] TRenderExtensionExpected<FRenderFramePassHandle> AddPass ( Bool bHasSideEffects ) noexcept override;
    [[nodiscard]] FRenderExtensionExpected Read ( const FRenderFramePassHandle InPass,
                                                  const FRenderFrameResourceHandle InResource ) noexcept override;
    [[nodiscard]] FRenderExtensionExpected Write ( const FRenderFramePassHandle InPass,
                                                   const FRenderFrameResourceHandle InResource ) noexcept override;
    [[nodiscard]] TUniquePtr<IRenderCompiledFrame> Compile () noexcept override;

private:

    friend class FRenderCompiledFrameAdapter;

    FRenderFeatureHost &Host;
    TOptional<FRenderGraphBuilder> Graph;
    TSharedPtr<FRenderTextureObject> Target;
    TSharedPtr<FRenderTextureObject> LastImported;
    TVector<TSharedPtr<FRenderTextureObject>> Imports;
};

/** @brief Compiled frame that records one pass with private barrier bookkeeping. */
class FRenderCompiledFrameAdapter final : public IRenderCompiledFrame
{
public:

    FRenderCompiledFrameAdapter ( FRenderFeatureHost &InHost,
                                  TVector<TSharedPtr<FRenderTextureObject>> InImports,
                                  TSpan<const FRenderTransition> InTransitions,
                                  FRenderGraphContext &&InContext ) noexcept;

    [[nodiscard]] FRenderExtensionExpected RecordPass ( IRenderPassEncoder &InEncoder,
                                                        UInt32 InPassOrdinal,
                                                        const FRenderPipelineRef &InPipeline,
                                                        const FRenderBindingSetRef &InBindingSet,
                                                        const FRenderTextureViewRef &InTargetView,
                                                        const FRenderExtent3D &InTargetExtent,
                                                        TSpan<const Byte> InConstants ) noexcept override;

private:

    [[nodiscard]] Bool RecordTransitions ( RHI::IRHICommandList &InCommandList,
                                           ERenderTransitionRecordPoint InPoint,
                                           UInt32 InPassOrdinal ) noexcept;

    FRenderFeatureHost &Host;
    TVector<TSharedPtr<FRenderTextureObject>> Imports;
    TVector<FRenderTransition> Transitions;
    FRenderGraphContext Context;
};

/** @brief Casts an opaque feature ref to its private implementation. */
template <typename TType> [[nodiscard]] TType *AsOpaque ( const TSharedPtr<IRenderOpaqueObject> &InRef ) noexcept;

} // namespace Lumen

#include "Inline/RenderFeatureHost.inl"
