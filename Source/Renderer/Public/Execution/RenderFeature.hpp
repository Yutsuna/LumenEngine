/**
 * @file RenderFeature.hpp
 * @brief Backend-neutral resource factories and frame-extension contract.
 */

#pragma once

#include <RendererExport.hpp>

#include <Core/CoreTypes.hpp>

#include <Execution/RenderFrameTypes.hpp>
#include <Frame/RenderFrameContracts.hpp>
#include <Memory/SharedPtr.hpp>
#include <Memory/Span.hpp>
#include <Memory/UniquePtr.hpp>
#include <Rendering/RenderSnapshotView.hpp>
#include <String/String.hpp>
#include <Templates/Expected.hpp>
#include <Templates/Vector.hpp>

namespace Lumen
{

class IRenderPassEncoder;
class IRenderFrameBuilder;
struct FRenderFrameCapacities;

/**
 * @enum ERenderFormat
 * @brief Backend-neutral pixel formats.
 */
enum class ERenderFormat : UInt8
{
    Unknown,
    RGBA8Unorm,
    RGBA16Float,
    RGBA32Float,
    RGB32Float,
};

/**
 * @enum ERenderShaderStage
 * @brief Shader stages exposed to features.
 */
enum class ERenderShaderStage : UInt8
{
    Vertex,
    Pixel,
    AllGraphics,
};

/**
 * @enum ERenderShaderBindingKind
 * @brief Kinds of inline shader binding.
 */
enum class ERenderShaderBindingKind : UInt8
{
    ConstantBuffer,
};

/**
 * @enum ERenderFilter
 * @brief Texture filtering modes.
 */
enum class ERenderFilter : UInt8
{
    Nearest,
    Linear,
};

/**
 * @enum ERenderAddressMode
 * @brief Texture addressing modes.
 */
enum class ERenderAddressMode : UInt8
{
    ClampToEdge,
};

/**
 * @enum ERenderBindingType
 * @brief Binding types exposed to features.
 */
enum class ERenderBindingType : UInt8
{
    CombinedTextureSampler,
};

/**
 * @enum ERenderPrimitiveTopology
 * @brief Primitive topologies exposed to features.
 */
enum class ERenderPrimitiveTopology : UInt8
{
    TriangleList,
};

/**
 * @enum ERenderResourceState
 * @brief Declarative states used by frame-graph access declarations.
 * @details Barrier bookkeeping is performed by Renderer/Private; features only declare intent.
 */
enum class ERenderResourceState : UInt8
{
    Undefined,
    Common,
    ShaderResource,
    RenderTarget,
    Present,
};

/**
 * @enum ERenderQueueType
 * @brief Queue kinds exposed to features.
 */
enum class ERenderQueueType : UInt8
{
    Graphics,
};

/**
 * @enum ERenderLoadOp
 * @brief Load operations exposed to the pass encoder.
 */
enum class ERenderLoadOp : UInt8
{
    DontCare,
    Clear,
};

/**
 * @enum ERenderStoreOp
 * @brief Store operations exposed to the pass encoder.
 */
enum class ERenderStoreOp : UInt8
{
    Store,
};

/**
 * @enum ERenderExtensionErrorCode
 * @brief Stable failure categories crossing the feature boundary.
 */
enum class ERenderExtensionErrorCode : UInt8
{
    InvalidDesc,
    CreationFailed,
    RecordFailed,
    CapabilityUnsupported,
};

/**
 * @struct FRenderExtensionError
 * @brief Structured, backend-independent feature failure.
 */
struct FRenderExtensionError final
{
    ERenderExtensionErrorCode Code = ERenderExtensionErrorCode::InvalidDesc;
    FString Message;
};

/** @brief Result of a void feature operation. */
using FRenderExtensionExpected = TExpected<void, FRenderExtensionError>;

/** @brief Result of a feature operation producing a value. */
template <typename TType> using TRenderExtensionExpected = TExpected<TType, FRenderExtensionError>;

/**
 * @class IRenderOpaqueObject
 * @brief Common base for all opaque renderer resources handed to features.
 */
class IRenderOpaqueObject
{
public:

    virtual ~IRenderOpaqueObject () noexcept = default;
};

using FRenderTextureRef        = TSharedPtr<IRenderOpaqueObject>;
using FRenderTextureViewRef    = TSharedPtr<IRenderOpaqueObject>;
using FRenderBufferRef         = TSharedPtr<IRenderOpaqueObject>;
using FRenderSamplerRef        = TSharedPtr<IRenderOpaqueObject>;
using FRenderShaderRef         = TSharedPtr<IRenderOpaqueObject>;
using FRenderPipelineRef       = TSharedPtr<IRenderOpaqueObject>;
using FRenderPipelineLayoutRef = TSharedPtr<IRenderOpaqueObject>;
using FRenderBindingLayoutRef  = TSharedPtr<IRenderOpaqueObject>;
using FRenderBindingSetRef     = TSharedPtr<IRenderOpaqueObject>;

/**
 * @struct FRenderShaderDesc
 * @brief Neutral shader creation inputs.
 */
struct FRenderShaderDesc final
{
    ERenderShaderStage Stage   = ERenderShaderStage::Pixel;
    ERenderShaderFormat Format = ERenderShaderFormat::Source;
    FString EntryPoint         = "main";
    TSpan<const Byte> Bytecode;
    USize InlineConstantBytes            = 0ULL;
    ERenderShaderBindingKind BindingKind = ERenderShaderBindingKind::ConstantBuffer;
};

/**
 * @struct FRenderSamplerDesc
 * @brief Neutral sampler creation inputs.
 */
struct FRenderSamplerDesc final
{
    ERenderFilter MinFilter     = ERenderFilter::Linear;
    ERenderFilter MagFilter     = ERenderFilter::Linear;
    ERenderFilter MipFilter     = ERenderFilter::Linear;
    ERenderAddressMode AddressU = ERenderAddressMode::ClampToEdge;
    ERenderAddressMode AddressV = ERenderAddressMode::ClampToEdge;
    ERenderAddressMode AddressW = ERenderAddressMode::ClampToEdge;
};

/**
 * @struct FRenderBindingLayoutDesc
 * @brief Neutral binding layout creation inputs.
 */
struct FRenderBindingLayoutDesc final
{
    struct FEntry final
    {
        UInt32 Slot               = 0U;
        ERenderBindingType Type   = ERenderBindingType::CombinedTextureSampler;
        ERenderShaderStage Stages = ERenderShaderStage::Pixel;
    };

    TVector<FEntry> Entries;
};

/**
 * @struct FRenderPipelineLayoutDesc
 * @brief Neutral pipeline layout creation inputs.
 */
struct FRenderPipelineLayoutDesc final
{
    struct FRange final
    {
        UInt32 Offset             = 0U;
        UInt32 Size               = 0U;
        ERenderShaderStage Stages = ERenderShaderStage::AllGraphics;
    };

    TVector<FRenderBindingLayoutRef> BindingLayouts;
    TVector<FRange> InlineConstants;
};

/**
 * @struct FRenderGraphicsPipelineDesc
 * @brief Neutral graphics pipeline creation inputs.
 */
struct FRenderGraphicsPipelineDesc final
{
    FRenderShaderRef VertexShader;
    FRenderShaderRef PixelShader;
    FRenderPipelineLayoutRef Layout;
    ERenderPrimitiveTopology Topology = ERenderPrimitiveTopology::TriangleList;
    TVector<ERenderFormat> ColorFormats;
};

/**
 * @struct FRenderTextureDesc
 * @brief Neutral texture creation inputs; usage and initial state are backend decisions.
 */
struct FRenderTextureDesc final
{
    FRenderExtent3D Extent{ 1U, 1U, 1U };
    ERenderFormat Format = ERenderFormat::RGBA8Unorm;
};

/**
 * @struct FRenderBindingWrite
 * @brief One combined texture/sampler binding write.
 */
struct FRenderBindingWrite final
{
    UInt32 Slot = 0U;
    FRenderTextureViewRef View;
    FRenderSamplerRef Sampler;
};

/**
 * @struct FRenderHostCapabilities
 * @brief Neutral capabilities exposed to features.
 */
struct FRenderHostCapabilities final
{
    /** INFO: True on a software adapter (Mesa llvmpipe in headless CI); features may halve their resolution. */
    Bool bSoftwareAdapter = false;
    FString DeviceName;
};

/**
 * @class IRenderFeatureHost
 * @brief Neutral resource factories implemented by Renderer/Private over the private RHI device.
 */
class RENDERER_EXPORT IRenderFeatureHost
{
public:

    virtual ~IRenderFeatureHost () noexcept = default;

    [[nodiscard]] virtual const FRenderHostCapabilities &GetCapabilities () const noexcept = 0;

    [[nodiscard]] virtual TRenderExtensionExpected<FRenderShaderRef>
    CreateShader ( const FRenderShaderDesc &InDesc ) noexcept = 0;
    [[nodiscard]] virtual TRenderExtensionExpected<FRenderSamplerRef>
    CreateSampler ( const FRenderSamplerDesc &InDesc ) noexcept = 0;
    [[nodiscard]] virtual TRenderExtensionExpected<FRenderBindingLayoutRef>
    CreateBindingLayout ( const FRenderBindingLayoutDesc &InDesc ) noexcept = 0;
    [[nodiscard]] virtual TRenderExtensionExpected<FRenderPipelineLayoutRef>
    CreatePipelineLayout ( const FRenderPipelineLayoutDesc &InDesc ) noexcept = 0;
    [[nodiscard]] virtual TRenderExtensionExpected<FRenderPipelineRef>
    CreateGraphicsPipeline ( const FRenderGraphicsPipelineDesc &InDesc ) noexcept = 0;
    [[nodiscard]] virtual TRenderExtensionExpected<FRenderTextureRef>
    CreateTexture ( const FRenderTextureDesc &InDesc ) noexcept = 0;
    [[nodiscard]] virtual TRenderExtensionExpected<FRenderTextureViewRef>
    CreateTextureView ( const FRenderTextureRef &InTexture ) noexcept = 0;
    [[nodiscard]] virtual TRenderExtensionExpected<FRenderBindingSetRef>
    CreateBindingSet ( const FRenderBindingLayoutRef &InLayout ) noexcept                                                 = 0;
    [[nodiscard]] virtual FRenderExtensionExpected WriteBindingSet ( const FRenderBindingSetRef &InSet,
                                                                     TSpan<const FRenderBindingWrite> InWrites ) noexcept = 0;

    /** @brief Creates a declarative frame builder; its definition lives in RenderFrameBuilder.hpp. */
    [[nodiscard]] virtual TUniquePtr<IRenderFrameBuilder>
    CreateFrameBuilder ( const FRenderFrameCapacities &InCapacities ) noexcept = 0;
};

/**
 * @struct FRenderFrameExtensionContext
 * @brief Borrowed render-thread inputs valid only for the current record call.
 */
struct FRenderFrameExtensionContext final
{
    const FRenderSnapshotView &Snapshot;
    IRenderFeatureHost &Host;
    IRenderPassEncoder &Encoder;
    FRenderTextureRef TargetTexture;
    FRenderTextureViewRef TargetView;
    FRenderExtent3D Extent;
    FRenderFrameStatus &Status;
};

/**
 * @class IRenderFeature
 * @brief Backend-neutral replacement for the former RHI-exposing frame extension.
 */
class RENDERER_EXPORT IRenderFeature
{
public:

    virtual ~IRenderFeature () noexcept = default;

    [[nodiscard]] virtual FRenderExtensionExpected Initialize ( IRenderFeatureHost &InHost,
                                                                const FRenderExtent3D &InExtent ) noexcept           = 0;
    [[nodiscard]] virtual FRenderExtensionExpected Resize ( const FRenderExtent3D &InExtent ) noexcept               = 0;
    [[nodiscard]] virtual FRenderExtensionExpected Record ( const FRenderFrameExtensionContext &InContext ) noexcept = 0;
    virtual void Commit ( UInt64 InTimelineValue, FRenderFrameStatus &InOutStatus ) noexcept                         = 0;
};

} // namespace Lumen
