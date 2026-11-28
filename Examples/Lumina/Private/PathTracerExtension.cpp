/**
 * @file PathTracerExtension.cpp
 * @brief Implements Lumina's portable path-tracing and post-processing passes.
 */

#include <PathTracerExtension.hpp>

#include <Filesystem/File.hpp>
#include <LogLumina.hpp>
#include <Maths/Macros.hpp>

#include <RHI/Bindings/IRHIBindingSet.hpp>
#include <RHI/Commands/IRHICommandList.hpp>
#include <RHI/Device/IRHIDevice.hpp>
#include <RHI/Pipelines/RHIGraphicsPipelineDesc.hpp>
#include <RHI/Pipelines/RHIPipelineLayoutDesc.hpp>
#include <RHI/Resources/IRHITexture.hpp>
#include <RHI/Resources/IRHITextureView.hpp>
#include <RHI/Resources/RHISamplerDesc.hpp>
#include <RHI/Resources/RHITextureDesc.hpp>
#include <RHI/Resources/RHITextureViewDesc.hpp>
#include <RHI/Shaders/RHIShaderDesc.hpp>
#include <RHI/Types/RHIShaderFormat.hpp>
#include <RHI/Types/RHIShaderStage.hpp>

#include <utility>

namespace
{

struct alignas( 16 ) FPathTraceConstants
{
    Lumen::Maths::FVec4f ResolutionFrame;
    Lumen::Maths::FVec4f CameraPosition;
    Lumen::Maths::FVec4f CameraForward;
    Lumen::Maths::FVec4f CameraRight;
    Lumen::Maths::FVec4f CameraUp;
    Lumen::Maths::FVec4f CameraParams;
};

struct alignas( 16 ) FPostProcessConstants
{
    Lumen::Maths::FVec4f Values;
};

static_assert( sizeof( FPathTraceConstants ) == 96U );
static_assert( sizeof( FPostProcessConstants ) == 16U );

[[nodiscard]] Lumen::RHI::FRHIError LuminaError ( const Lumen::RHI::ERHIErrorCondition InCondition,
                                                  const Lumen::FStringView InMessage ) noexcept
{
    return Lumen::RHI::MakeRHIBackendFailure( InCondition, InMessage );
}

[[nodiscard]] Lumen::RHI::FRHIExpected GraphError ( const Lumen::FRenderExpected &InResult ) noexcept
{
    if ( not InResult )
    {
        LUMEN_LOG_ERROR( Lumen::LogLumina, "Render graph declaration error {} at pass {}, resource {}, related pass {}.",
                         static_cast<Lumen::UInt32>( InResult.Error().Code ), InResult.Error().PassOrdinal,
                         InResult.Error().ResourceIndex, InResult.Error().RelatedPassOrdinal );
        return Lumen::MakeUnexpected( LuminaError( Lumen::RHI::ERHIErrorCondition::Command,
                                                  "Lumina render graph declaration failed." ) );
    }
    return {};
}

template <typename TType>
[[nodiscard]] Lumen::TExpected<TType, Lumen::RHI::FRHIError>
GraphValue ( Lumen::TExpected<TType, Lumen::FRenderError> InResult ) noexcept
{
    if ( not InResult )
    {
        LUMEN_LOG_ERROR( Lumen::LogLumina, "Render graph value error {} at pass {}, resource {}, related pass {}.",
                         static_cast<Lumen::UInt32>( InResult.Error().Code ), InResult.Error().PassOrdinal,
                         InResult.Error().ResourceIndex, InResult.Error().RelatedPassOrdinal );
        return Lumen::MakeUnexpected( LuminaError( Lumen::RHI::ERHIErrorCondition::Command,
                                                  "Lumina render graph compilation failed." ) );
    }
    return std::move( *InResult );
}

[[nodiscard]] Lumen::TExpected<Lumen::FString, Lumen::RHI::FRHIError> LoadShader ( const Lumen::FStringView InFilename ) noexcept
{
    constexpr const Lumen::AnsiChar *Roots[] = {
        "Examples/Lumina/Shaders",
        "../Examples/Lumina/Shaders",
        "../../Examples/Lumina/Shaders",
        "../../../Examples/Lumina/Shaders",
#if defined( LUMINA_SHADER_INSTALL_DIR )
        LUMINA_SHADER_INSTALL_DIR,
#endif
    };
    const Lumen::FString Filename( InFilename );
    for ( const Lumen::AnsiChar *const Root : Roots )
    {
        const Lumen::Filesystem::FPath Path = Lumen::Filesystem::FPath( Root ) / Filename;
        if ( Lumen::Filesystem::FFile::Exists( Path ) )
        {
            auto Source = Lumen::Filesystem::FFile::ReadAllText( Path );
            if ( not Source )
            {
                return Lumen::MakeUnexpected( LuminaError( Lumen::RHI::ERHIErrorCondition::Shader,
                                                          "Unable to read a Lumina shader source file." ) );
            }
            return std::move( *Source );
        }
    }
    return Lumen::MakeUnexpected( LuminaError( Lumen::RHI::ERHIErrorCondition::Shader,
                                              "Lumina shader directory was not found." ) );
}

[[nodiscard]] Lumen::TExpected<Lumen::TSharedRef<Lumen::RHI::IRHITexture>, Lumen::RHI::FRHIError>
CreateTexture ( Lumen::RHI::IRHIDevice &InDevice,
                const Lumen::RHI::FRHIExtent3D &InExtent,
                const Lumen::RHI::ERHIFormat InFormat ) noexcept
{
    return InDevice.CreateTexture( { .Extent = InExtent,
                                     .Format = InFormat,
                                     .Usage  = Lumen::RHI::ERHIResourceUsage::RenderTarget |
                                               Lumen::RHI::ERHIResourceUsage::ShaderResource,
                                     .InitialState = Lumen::RHI::ERHIResourceState::Common,
                                     .OptimizedClear = {} } );
}

[[nodiscard]] Lumen::TExpected<Lumen::TSharedRef<Lumen::RHI::IRHITextureView>, Lumen::RHI::FRHIError>
CreateView ( Lumen::RHI::IRHIDevice &InDevice, const Lumen::TSharedRef<Lumen::RHI::IRHITexture> &InTexture ) noexcept
{
    Lumen::RHI::FRHITextureViewDesc Desc;
    Desc.Texture = InTexture;
    return InDevice.CreateTextureView( Desc );
}

[[nodiscard]] Lumen::RHI::FRHIExpected WriteOneTexture ( Lumen::RHI::IRHIBindingSet &InSet,
                                                         const Lumen::TSharedRef<Lumen::RHI::IRHITextureView> &InView,
                                                         const Lumen::TSharedPtr<Lumen::RHI::IRHISampler> &InSampler,
                                                         const Lumen::UInt32 InSlot = 0U ) noexcept
{
    const Lumen::RHI::FRHIBindingWrite Write{
        .Slot     = InSlot,
        .Resource = Lumen::RHI::FRHICombinedTextureSamplerBinding{ .View = InView, .Sampler = InSampler },
    };
    return InSet.Write( Lumen::TSpan<const Lumen::RHI::FRHIBindingWrite>( &Write, 1U ) );
}

} // namespace

Lumen::FLuminaPathTracerExtension::FLuminaPathTracerExtension ( const Bool bInSoftwareAdapter ) noexcept
    : bSoftwareAdapter( bInSoftwareAdapter )
{
    AccumulationStates.fill( RHI::ERHIResourceState::Common );
    BloomStates.fill( RHI::ERHIResourceState::Common );
}

Lumen::RHI::FRHIExtent3D
Lumen::FLuminaPathTracerExtension::ScaleExtent ( const RHI::FRHIExtent3D &InExtent ) const noexcept
{
    const UInt32 Width  = bSoftwareAdapter ? InExtent.Width / 2U : InExtent.Width;
    const UInt32 Height = bSoftwareAdapter ? InExtent.Height / 2U : InExtent.Height;
    return { Width > 0U ? Width : 1U, Height > 0U ? Height : 1U, 1U };
}

Lumen::TExpected<Lumen::TSharedRef<Lumen::RHI::IRHIPipeline>, Lumen::RHI::FRHIError>
Lumen::FLuminaPathTracerExtension::CreatePipeline ( const FString &InFragmentSource,
                                                    const UInt32 InInlineConstantBytes,
                                                    TSharedRef<RHI::IRHIBindingLayout> InBindingLayout,
                                                    const RHI::ERHIFormat InTargetFormat ) noexcept
{
    RHI::FRHIShaderDesc PixelDesc;
    PixelDesc.Stage       = RHI::ERHIShaderStage::Pixel;
    PixelDesc.Format      = RHI::ERHIShaderFormat::Source;
    PixelDesc.EntryPoint  = "main";
    PixelDesc.Bytecode    = TSpan<const Byte>( reinterpret_cast<const Byte *>( InFragmentSource.data() ), InFragmentSource.size() );
    PixelDesc.Reflection.InlineConstantBytes = InInlineConstantBytes;
    PixelDesc.Reflection.Bindings.push_back( { .Slot   = 0U,
                                               .Kind   = RHI::ERHIShaderBindingKind::ConstantBuffer,
                                               .Stages = RHI::ERHIShaderStage::Pixel } );
    LUMEN_EXPECT( PixelShader, Device->CreateShader( PixelDesc ) );

    RHI::FRHIPipelineLayoutDesc LayoutDesc;
    LayoutDesc.BindingLayouts.push_back( InBindingLayout );
    LayoutDesc.InlineConstants.push_back( { .Offset = 0U,
                                            .Size   = InInlineConstantBytes,
                                            .Stages = RHI::ERHIShaderStage::AllGraphics } );
    LUMEN_EXPECT( Layout, Device->CreatePipelineLayout( LayoutDesc ) );

    RHI::FRHIGraphicsPipelineDesc PipelineDesc;
    PipelineDesc.VertexShader = *VertexShader;
    PipelineDesc.PixelShader  = PixelShader;
    PipelineDesc.Layout       = Layout;
    PipelineDesc.Topology     = RHI::ERHIPrimitiveTopology::TriangleList;
    PipelineDesc.ColorFormats.push_back( InTargetFormat );
    return Device->CreateGraphicsPipeline( PipelineDesc );
}

Lumen::RHI::FRHIExpected
Lumen::FLuminaPathTracerExtension::Initialize ( RHI::IRHIDevice &InDevice, const RHI::FRHIExtent3D &InExtent ) noexcept
{
    Device = &InDevice;
    LUMEN_EXPECT( LoadedVertex, LoadShader( "Fullscreen.vert" ) );
    LUMEN_EXPECT( LoadedPathTrace, LoadShader( "PathTrace.frag" ) );
    LUMEN_EXPECT( LoadedBloomExtract, LoadShader( "BloomExtract.frag" ) );
    LUMEN_EXPECT( LoadedBloomBlur, LoadShader( "BloomBlur.frag" ) );
    LUMEN_EXPECT( LoadedDisplay, LoadShader( "Display.frag" ) );
    VertexSource       = std::move( LoadedVertex );
    PathTraceSource    = std::move( LoadedPathTrace );
    BloomExtractSource = std::move( LoadedBloomExtract );
    BloomBlurSource    = std::move( LoadedBloomBlur );
    DisplaySource      = std::move( LoadedDisplay );

    RHI::FRHIShaderDesc VertexDesc;
    VertexDesc.Stage      = RHI::ERHIShaderStage::Vertex;
    VertexDesc.Format     = RHI::ERHIShaderFormat::Source;
    VertexDesc.EntryPoint = "main";
    VertexDesc.Bytecode   = TSpan<const Byte>( reinterpret_cast<const Byte *>( VertexSource.data() ), VertexSource.size() );
    LUMEN_EXPECT( CreatedVertexShader, Device->CreateShader( VertexDesc ) );
    VertexShader.Emplace( CreatedVertexShader );

    RHI::FRHISamplerDesc SamplerDesc;
    SamplerDesc.MinFilter = RHI::ERHISamplerFilter::Nearest;
    SamplerDesc.MagFilter = RHI::ERHISamplerFilter::Nearest;
    SamplerDesc.MipFilter = RHI::ERHISamplerFilter::Nearest;
    SamplerDesc.AddressU  = RHI::ERHISamplerAddressMode::ClampToEdge;
    SamplerDesc.AddressV  = RHI::ERHISamplerAddressMode::ClampToEdge;
    SamplerDesc.AddressW  = RHI::ERHISamplerAddressMode::ClampToEdge;
    LUMEN_EXPECT( CreatedNearestSampler, Device->CreateSampler( SamplerDesc ) );
    NearestSampler = CreatedNearestSampler;
    SamplerDesc.MinFilter = RHI::ERHISamplerFilter::Linear;
    SamplerDesc.MagFilter = RHI::ERHISamplerFilter::Linear;
    SamplerDesc.MipFilter = RHI::ERHISamplerFilter::Linear;
    LUMEN_EXPECT( CreatedLinearSampler, Device->CreateSampler( SamplerDesc ) );
    LinearSampler = CreatedLinearSampler;

    RHI::FRHIBindingLayoutDesc OneTextureDesc;
    OneTextureDesc.Entries.push_back( { .Slot   = 0U,
                                        .Type   = RHI::ERHIBindingType::CombinedTextureSampler,
                                        .Stages = RHI::ERHIShaderStage::Pixel } );
    LUMEN_EXPECT( CreatedOneTextureLayout, Device->CreateBindingLayout( OneTextureDesc ) );
    OneTextureLayout.Emplace( CreatedOneTextureLayout );
    RHI::FRHIBindingLayoutDesc TwoTextureDesc = OneTextureDesc;
    TwoTextureDesc.Entries.push_back( { .Slot   = 1U,
                                        .Type   = RHI::ERHIBindingType::CombinedTextureSampler,
                                        .Stages = RHI::ERHIShaderStage::Pixel } );
    LUMEN_EXPECT( CreatedTwoTextureLayout, Device->CreateBindingLayout( TwoTextureDesc ) );
    TwoTextureLayout.Emplace( CreatedTwoTextureLayout );

    LUMEN_EXPECT( CreatedPathTrace,
                  CreatePipeline( PathTraceSource, sizeof( FPathTraceConstants ), *OneTextureLayout, RHI::ERHIFormat::RGBA32Float ) );
    LUMEN_EXPECT( CreatedBloomExtract,
                  CreatePipeline( BloomExtractSource, sizeof( FPostProcessConstants ), *OneTextureLayout, RHI::ERHIFormat::RGBA16Float ) );
    LUMEN_EXPECT( CreatedBloomBlur,
                  CreatePipeline( BloomBlurSource, sizeof( FPostProcessConstants ), *OneTextureLayout, RHI::ERHIFormat::RGBA16Float ) );
    LUMEN_EXPECT( CreatedDisplay,
                  CreatePipeline( DisplaySource, sizeof( FPostProcessConstants ), *TwoTextureLayout, RHI::ERHIFormat::RGBA8Unorm ) );
    PathTracePipeline.Emplace( CreatedPathTrace );
    BloomExtractPipeline.Emplace( CreatedBloomExtract );
    BloomBlurPipeline.Emplace( CreatedBloomBlur );
    DisplayPipeline.Emplace( CreatedDisplay );

    RHI::FRHIRenderingAttachment ColorAttachment{};
    ColorAttachment.LoadOp = RHI::ERHIRenderingLoadOp::DontCare;
    RenderingDesc.ColorAttachments.push_back( ColorAttachment );
    GraphBuilder.Emplace( FRenderGraphCapacities{ .PassCount          = 7U,
                                                  .ResourceCount      = 5U,
                                                  .ReadCountPerPass   = 2U,
                                                  .WriteCountPerPass  = 1U,
                                                  .DependencyCountPerPass = 7U,
                                                  .TransitionCount    = 64U } );
    TransitionRecords.reserve( 64U );
    return CreateFrameResources( ScaleExtent( InExtent ) );
}

Lumen::RHI::FRHIExpected
Lumen::FLuminaPathTracerExtension::CreateFrameResources ( const RHI::FRHIExtent3D &InExtent ) noexcept
{
    AccumulationExtent = InExtent;
    BloomExtent        = { InExtent.Width / 4U > 0U ? InExtent.Width / 4U : 1U,
                           InExtent.Height / 4U > 0U ? InExtent.Height / 4U : 1U,
                           1U };

    AccumulationTextures.clear();
    AccumulationViews.clear();
    BloomTextures.clear();
    BloomViews.clear();
    PathTraceBindings.clear();
    BloomExtractBindings.clear();
    DisplayBindings.clear();
    AccumulationTextures.reserve( 2U );
    AccumulationViews.reserve( 2U );
    BloomTextures.reserve( 2U );
    BloomViews.reserve( 2U );
    PathTraceBindings.reserve( 2U );
    BloomExtractBindings.reserve( 2U );
    DisplayBindings.reserve( 2U );

    for ( UInt32 Index = 0U; Index < 2U; ++Index )
    {
        LUMEN_EXPECT( AccumulationTexture, CreateTexture( *Device, AccumulationExtent, RHI::ERHIFormat::RGBA32Float ) );
        LUMEN_EXPECT( AccumulationView, CreateView( *Device, AccumulationTexture ) );
        AccumulationTextures.push_back( AccumulationTexture );
        AccumulationViews.push_back( AccumulationView );

        LUMEN_EXPECT( BloomTexture, CreateTexture( *Device, BloomExtent, RHI::ERHIFormat::RGBA16Float ) );
        LUMEN_EXPECT( BloomView, CreateView( *Device, BloomTexture ) );
        BloomTextures.push_back( BloomTexture );
        BloomViews.push_back( BloomView );

        LUMEN_EXPECT( PathTraceBinding, Device->CreateBindingSet( *OneTextureLayout ) );
        LUMEN_TRY_EXPECT_ERROR( WriteOneTexture( *PathTraceBinding, AccumulationView, NearestSampler ) );
        PathTraceBindings.push_back( PathTraceBinding );

        LUMEN_EXPECT( BloomExtractBinding, Device->CreateBindingSet( *OneTextureLayout ) );
        LUMEN_TRY_EXPECT_ERROR( WriteOneTexture( *BloomExtractBinding, AccumulationView, NearestSampler ) );
        BloomExtractBindings.push_back( BloomExtractBinding );

        LUMEN_EXPECT( DisplayBinding, Device->CreateBindingSet( *TwoTextureLayout ) );
        DisplayBindings.push_back( DisplayBinding );
    }

    LUMEN_EXPECT( CreatedBlurHorizontal, Device->CreateBindingSet( *OneTextureLayout ) );
    LUMEN_TRY_EXPECT_ERROR( WriteOneTexture( *CreatedBlurHorizontal, BloomViews[0], LinearSampler ) );
    BlurHorizontalBinding.Emplace( CreatedBlurHorizontal );
    LUMEN_EXPECT( CreatedBlurVertical, Device->CreateBindingSet( *OneTextureLayout ) );
    LUMEN_TRY_EXPECT_ERROR( WriteOneTexture( *CreatedBlurVertical, BloomViews[1], LinearSampler ) );
    BlurVerticalBinding.Emplace( CreatedBlurVertical );

    for ( UInt32 Index = 0U; Index < 2U; ++Index )
    {
        const RHI::FRHIBindingWrite DisplayWrites[] = {
            { .Slot = 0U, .Resource = RHI::FRHICombinedTextureSamplerBinding{ .View = AccumulationViews[Index], .Sampler = NearestSampler } },
            { .Slot = 1U, .Resource = RHI::FRHICombinedTextureSamplerBinding{ .View = BloomViews[0], .Sampler = LinearSampler } },
        };
        LUMEN_TRY_EXPECT_ERROR( DisplayBindings[Index]->Write( DisplayWrites ) );
    }

    AccumulationStates.fill( RHI::ERHIResourceState::Common );
    BloomStates.fill( RHI::ERHIResourceState::Common );
    SampleCount = 0ULL;
    PingPong    = 0U;
    return {};
}

Lumen::RHI::FRHIExpected Lumen::FLuminaPathTracerExtension::Resize ( const RHI::FRHIExtent3D &InExtent ) noexcept
{
    const RHI::FRHIExtent3D Scaled = ScaleExtent( InExtent );
    if ( Scaled != AccumulationExtent )
    {
        LUMEN_TRY_EXPECT_ERROR( CreateFrameResources( Scaled ) );
    }
    SampleCount = 0ULL;
    PingPong    = 0U;
    return {};
}

Lumen::RHI::FRHIExpected
Lumen::FLuminaPathTracerExtension::RecordTransitions ( const FRenderGraphContext &InGraph,
                                                       RHI::IRHICommandList &InCommandList,
                                                       const UInt32 InPassOrdinal,
                                                       const ERenderTransitionRecordPoint InPoint ) noexcept
{
    for ( const FTransitionRecord &Record : TransitionRecords )
    {
        if ( Record.PassOrdinal != InPassOrdinal or Record.Point != InPoint )
        {
            continue;
        }
        LUMEN_EXPECT( Transition, GraphValue( InGraph.ResolveTransition( Record.TransitionIndex ) ) );
        LUMEN_TRY_EXPECT_ERROR( InCommandList.Transition( TSpan<const RHI::FRHITransition>( &Transition, 1U ) ) );
    }
    return {};
}

Lumen::RHI::FRHIExpected
Lumen::FLuminaPathTracerExtension::RecordDraw ( const FRenderGraphContext &InGraph,
                                                RHI::IRHICommandList &InCommandList,
                                                const UInt32 InPassOrdinal,
                                                TSharedRef<RHI::IRHIPipeline> InPipeline,
                                                TSharedRef<RHI::IRHIBindingSet> InBindingSet,
                                                TSharedRef<RHI::IRHITextureView> InTargetView,
                                                const RHI::FRHIExtent3D &InTargetExtent,
                                                const TSpan<const Byte> InConstants ) noexcept
{
    LUMEN_TRY_EXPECT_ERROR( RecordTransitions( InGraph, InCommandList, InPassOrdinal, ERenderTransitionRecordPoint::BeforePass ) );
    RenderingDesc.Area = RHI::FRHIRect2D{ 0, 0, InTargetExtent.Width, InTargetExtent.Height };
    RenderingDesc.ColorAttachments[0].View = InTargetView;
    LUMEN_TRY_EXPECT_ERROR( InCommandList.BeginRendering( RenderingDesc ) );
    LUMEN_TRY_EXPECT_ERROR( InCommandList.SetPipeline( InPipeline ) );
    LUMEN_TRY_EXPECT_ERROR( InCommandList.SetBindingSet( 0U, InBindingSet ) );
    LUMEN_TRY_EXPECT_ERROR( InCommandList.SetInlineConstants( RHI::ERHIShaderStage::Pixel, 0U, InConstants ) );
    LUMEN_TRY_EXPECT_ERROR( InCommandList.Draw( { .VertexCount = 3U } ) );
    LUMEN_TRY_EXPECT_ERROR( InCommandList.EndRendering() );
    return RecordTransitions( InGraph, InCommandList, InPassOrdinal, ERenderTransitionRecordPoint::AfterPass );
}

Lumen::RHI::FRHIExpected
Lumen::FLuminaPathTracerExtension::Record ( const FRenderFrameExtensionContext &InContext ) noexcept
{
    if ( Device == nullptr or not GraphBuilder or AccumulationTextures.size() != 2U )
    {
        return MakeUnexpected( LuminaError( RHI::ERHIErrorCondition::Command, "Lumina frame resources are not initialized." ) );
    }

    const FRenderCameraRecord *ActiveCamera = nullptr;
    for ( const FRenderCameraRecord &Camera : InContext.Snapshot.Cameras )
    {
        if ( HasAnyFlags( Camera.Flags, ERenderCameraFlags::Active ) and
             ( ActiveCamera == nullptr or Camera.ProxyId < ActiveCamera->ProxyId ) )
        {
            ActiveCamera = &Camera;
        }
    }
    const FRenderTransformRecord *CameraTransform = nullptr;
    if ( ActiveCamera != nullptr )
    {
        for ( const FRenderTransformRecord &Transform : InContext.Snapshot.Transforms )
        {
            if ( Transform.ProxyId == ActiveCamera->ProxyId )
            {
                CameraTransform = &Transform;
                break;
            }
        }
    }
    if ( CameraTransform == nullptr )
    {
        return MakeUnexpected( LuminaError( RHI::ERHIErrorCondition::Command, "The active Lumina camera has no transform." ) );
    }

    const Maths::FMat4x4f &WorldTransform = CameraTransform->WorldTransform;
    const Maths::FVec3f CameraPosition( WorldTransform[3].X, WorldTransform[3].Y, WorldTransform[3].Z );

    /**
     * INFO: LookAt stores R = Up x Forward in column 0, while PathTrace.frag builds rays with
     *       Forward + Right * ndc.x + Up * ndc.y. That shader basis is left-handed, so Right is -R.
     */
    const Maths::FVec3f Forward( WorldTransform[2].X, WorldTransform[2].Y, WorldTransform[2].Z );
    const Maths::FVec3f Up( WorldTransform[1].X, WorldTransform[1].Y, WorldTransform[1].Z );
    const Maths::FVec3f Right( -WorldTransform[0].X, -WorldTransform[0].Y, -WorldTransform[0].Z );

    /** INFO: Perspective stores +/- 1 / tan(fov / 2) in [1][1]; the magnitude is what the shader needs. */
    const Float32 ProjectionScale = ActiveCamera->Projection[1].Data[1];
    const Float32 TanHalfFov      = ProjectionScale != 0.0F ? 1.0F / Maths::Abs( ProjectionScale ) : 1.0F;

    /** INFO: A rotation-only look-at or a FOV change must restart accumulation just like a translation does. */
    if ( not bHasCamera or CameraPosition != LastCameraPosition or Forward != LastCameraForward or
         TanHalfFov != LastTanHalfFov )
    {
        SampleCount = 0ULL;
        PingPong    = 0U;
    }
    if ( not bHasCamera )
    {
        LUMEN_LOG_INFO( LogLumina, "Initial path-traced camera position: ({}, {}, {}).", CameraPosition.X, CameraPosition.Y,
                        CameraPosition.Z );
    }
    bHasCamera         = true;
    LastCameraPosition = CameraPosition;
    LastCameraForward  = Forward;
    LastTanHalfFov     = TanHalfFov;
    PendingWriteIndex  = 1U - PingPong;
    const UInt32 ReadIndex  = PingPong;
    const UInt32 WriteIndex = PendingWriteIndex;
    InContext.Status.ExtensionFrameCount = SampleCount;
    const FPathTraceConstants PathConstants{
        .ResolutionFrame = Maths::FVec4f( static_cast<Float32>( AccumulationExtent.Width ),
                                          static_cast<Float32>( AccumulationExtent.Height ),
                                          static_cast<Float32>( SampleCount ), 0.0F ),
        .CameraPosition  = Maths::FVec4f( CameraPosition.X, CameraPosition.Y, CameraPosition.Z, 0.0F ),
        .CameraForward   = Maths::FVec4f( Forward.X, Forward.Y, Forward.Z, 0.0F ),
        .CameraRight     = Maths::FVec4f( Right.X, Right.Y, Right.Z, 0.0F ),
        .CameraUp        = Maths::FVec4f( Up.X, Up.Y, Up.Z, 0.0F ),
        .CameraParams    = Maths::FVec4f( TanHalfFov, 0.0F, 0.0F, 0.0F ),
    };
    const FPostProcessConstants NextSampleConstants{ .Values = Maths::FVec4f( static_cast<Float32>( SampleCount + 1ULL ),
                                                                             0.0F, 0.0F, 0.0F ) };

    GraphBuilder->Reset();
    const auto Import = [this] ( TSharedRef<RHI::IRHITexture> InTexture, const RHI::ERHIResourceState InState,
                                 const RHI::ERHIResourceState InFinalState ) noexcept
    {
        return GraphValue( GraphBuilder->Import( InTexture, { .InitialState = InState,
                                                              .FinalState   = InFinalState,
                                                              .InitialQueue = RHI::ERHIQueueType::Graphics,
                                                              .FinalQueue   = RHI::ERHIQueueType::Graphics } ) );
    };
    LUMEN_EXPECT( AccumReadResource, Import( AccumulationTextures[ReadIndex], AccumulationStates[ReadIndex],
                                             RHI::ERHIResourceState::ShaderResource ) );
    LUMEN_EXPECT( AccumWriteResource, Import( AccumulationTextures[WriteIndex], AccumulationStates[WriteIndex],
                                              RHI::ERHIResourceState::ShaderResource ) );
    LUMEN_EXPECT( Bloom0Resource, Import( BloomTextures[0], BloomStates[0], RHI::ERHIResourceState::ShaderResource ) );
    LUMEN_EXPECT( Bloom1Resource, Import( BloomTextures[1], BloomStates[1], RHI::ERHIResourceState::ShaderResource ) );
    const RHI::ERHIResourceState TargetState = InContext.TargetTexture->GetInitialState();
    const RHI::ERHIResourceState FinalTargetState =
        TargetState == RHI::ERHIResourceState::Present ? RHI::ERHIResourceState::Present : RHI::ERHIResourceState::Common;
    LUMEN_EXPECT( TargetResource, Import( InContext.TargetTexture, TargetState, FinalTargetState ) );

    TArray<FRenderPassHandle, 7U> Passes{};
    for ( FRenderPassHandle &Pass : Passes )
    {
        LUMEN_EXPECT( CreatedPass, GraphValue( GraphBuilder->AddPass( { .bHasSideEffects = true } ) ) );
        Pass = CreatedPass;
    }
    LUMEN_TRY_EXPECT_ERROR( GraphError( GraphBuilder->Read( Passes[0], AccumReadResource, RHI::ERHIResourceState::ShaderResource ) ) );
    LUMEN_TRY_EXPECT_ERROR( GraphError( GraphBuilder->Write( Passes[0], AccumWriteResource, RHI::ERHIResourceState::RenderTarget ) ) );
    LUMEN_TRY_EXPECT_ERROR( GraphError( GraphBuilder->Read( Passes[1], AccumWriteResource, RHI::ERHIResourceState::ShaderResource ) ) );
    LUMEN_TRY_EXPECT_ERROR( GraphError( GraphBuilder->Write( Passes[1], Bloom0Resource, RHI::ERHIResourceState::RenderTarget ) ) );
    for ( UInt32 BlurPass = 0U; BlurPass < 4U; ++BlurPass )
    {
        const Bool bHorizontal = ( BlurPass % 2U ) == 0U;
        LUMEN_TRY_EXPECT_ERROR( GraphError( GraphBuilder->Read( Passes[2U + BlurPass],
                                                                bHorizontal ? Bloom0Resource : Bloom1Resource,
                                                                RHI::ERHIResourceState::ShaderResource ) ) );
        LUMEN_TRY_EXPECT_ERROR( GraphError( GraphBuilder->Write( Passes[2U + BlurPass],
                                                                 bHorizontal ? Bloom1Resource : Bloom0Resource,
                                                                 RHI::ERHIResourceState::RenderTarget ) ) );
    }
    LUMEN_TRY_EXPECT_ERROR( GraphError( GraphBuilder->Read( Passes[6], AccumWriteResource, RHI::ERHIResourceState::ShaderResource ) ) );
    LUMEN_TRY_EXPECT_ERROR( GraphError( GraphBuilder->Read( Passes[6], Bloom0Resource, RHI::ERHIResourceState::ShaderResource ) ) );
    LUMEN_TRY_EXPECT_ERROR( GraphError( GraphBuilder->Write( Passes[6], TargetResource, RHI::ERHIResourceState::RenderTarget ) ) );

    LUMEN_EXPECT( Compiled, GraphValue( std::move( *GraphBuilder ).Compile() ) );
    TransitionRecords.clear();
    const TSpan<const FRenderTransition> Transitions = Compiled.GetTransitions();
    for ( USize Index = 0U; Index < Transitions.size(); ++Index )
    {
        TransitionRecords.push_back( { .TransitionIndex = Index,
                                       .PassOrdinal     = Transitions[Index].RecordPassOrdinal,
                                       .Point           = Transitions[Index].RecordPoint } );
    }
    FRenderGraphContext Graph( std::move( Compiled ) );

    for ( UInt32 PassOrdinal = 0U; PassOrdinal < 7U; ++PassOrdinal )
    {
        RHI::FRHIExpected Recorded{};
        switch ( PassOrdinal )
        {
        case 0U:
            Recorded = RecordDraw( Graph, InContext.CommandList, PassOrdinal, *PathTracePipeline, PathTraceBindings[ReadIndex],
                                   AccumulationViews[WriteIndex], AccumulationExtent,
                                   TSpan<const Byte>( reinterpret_cast<const Byte *>( &PathConstants ), sizeof( PathConstants ) ) );
            break;
        case 1U:
            Recorded = RecordDraw( Graph, InContext.CommandList, PassOrdinal, *BloomExtractPipeline,
                                   BloomExtractBindings[WriteIndex], BloomViews[0], BloomExtent,
                                   TSpan<const Byte>( reinterpret_cast<const Byte *>( &NextSampleConstants ),
                                                      sizeof( NextSampleConstants ) ) );
            break;
        case 2U:
        case 4U:
        {
            const Float32 Scale = PassOrdinal == 2U ? 1.0F : 2.2F;
            const FPostProcessConstants Constants{ .Values = Maths::FVec4f( Scale, 0.0F, 0.0F, 0.0F ) };
            Recorded = RecordDraw( Graph, InContext.CommandList, PassOrdinal, *BloomBlurPipeline, *BlurHorizontalBinding,
                                   BloomViews[1], BloomExtent,
                                   TSpan<const Byte>( reinterpret_cast<const Byte *>( &Constants ), sizeof( Constants ) ) );
            break;
        }
        case 3U:
        case 5U:
        {
            const Float32 Scale = PassOrdinal == 3U ? 1.0F : 2.2F;
            const FPostProcessConstants Constants{ .Values = Maths::FVec4f( 0.0F, Scale, 0.0F, 0.0F ) };
            Recorded = RecordDraw( Graph, InContext.CommandList, PassOrdinal, *BloomBlurPipeline, *BlurVerticalBinding,
                                   BloomViews[0], BloomExtent,
                                   TSpan<const Byte>( reinterpret_cast<const Byte *>( &Constants ), sizeof( Constants ) ) );
            break;
        }
        case 6U:
        {
            const FPostProcessConstants Constants{ .Values = Maths::FVec4f( static_cast<Float32>( SampleCount + 1ULL ),
                                                                          static_cast<Float32>( InContext.Extent.Width ),
                                                                          static_cast<Float32>( InContext.Extent.Height ),
                                                                          0.0F ) };
            Recorded = RecordDraw( Graph, InContext.CommandList, PassOrdinal, *DisplayPipeline, DisplayBindings[WriteIndex],
                                   InContext.TargetView, InContext.Extent,
                                   TSpan<const Byte>( reinterpret_cast<const Byte *>( &Constants ), sizeof( Constants ) ) );
            break;
        }
        default:
            break;
        }
        if ( not Recorded )
        {
            return Recorded;
        }
        ++InContext.Status.DrawCallCount;
    }

    PendingAccumulationStates.fill( RHI::ERHIResourceState::ShaderResource );
    PendingBloomStates.fill( RHI::ERHIResourceState::ShaderResource );
    return {};
}

void Lumen::FLuminaPathTracerExtension::Commit ( const UInt64 LUMEN_UNUSED InTimelineValue,
                                                 FRenderFrameStatus &InOutStatus ) noexcept
{
    AccumulationStates = PendingAccumulationStates;
    BloomStates        = PendingBloomStates;
    PingPong           = PendingWriteIndex;
    ++SampleCount;
    InOutStatus.ExtensionFrameCount = SampleCount;
}
