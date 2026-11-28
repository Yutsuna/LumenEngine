/**
 * @file PathTracerExtension.cpp
 * @brief Implements Lumina's portable path-tracing and post-processing passes.
 */

#include <PathTracerExtension.hpp>

#include <Filesystem/File.hpp>
#include <LogLumina.hpp>
#include <Maths/Macros.hpp>

#include <Templates/Unexpected.hpp>

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

[[nodiscard]] Lumen::FRenderExtensionError LuminaError ( const Lumen::FStringView InMessage ) noexcept
{
    return { Lumen::ERenderExtensionErrorCode::RecordFailed, Lumen::FString( InMessage ) };
}

[[nodiscard]] Lumen::FRenderExtensionExpected GraphError ( const Lumen::FRenderExtensionExpected &InResult ) noexcept
{
    if ( not InResult )
    {
        LUMEN_LOG_ERROR( Lumen::LogLumina, "Lumina render graph declaration failed: {}", InResult.Error().Message );
        return Lumen::MakeUnexpected( InResult.Error() );
    }
    return {};
}

[[nodiscard]] Lumen::TExpected<Lumen::FString, Lumen::FRenderExtensionError>
LoadShader ( const Lumen::FStringView InFilename ) noexcept
{
    constexpr const Lumen::AnsiChar *Roots[] = {
        "Examples/Lumina/Shaders",       "../Examples/Lumina/Shaders",
        "../../Examples/Lumina/Shaders", "../../../Examples/Lumina/Shaders",
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
                return Lumen::MakeUnexpected( LuminaError( "Unable to read a Lumina shader source file." ) );
            }
            return std::move( *Source );
        }
    }
    return Lumen::MakeUnexpected( LuminaError( "Lumina shader directory was not found." ) );
}

} // namespace

Lumen::FRenderExtent3D Lumen::FLuminaPathTracerExtension::ScaleExtent ( const FRenderExtent3D &InExtent ) const noexcept
{
    const UInt32 Width  = bSoftwareAdapter ? InExtent.Width / 2U : InExtent.Width;
    const UInt32 Height = bSoftwareAdapter ? InExtent.Height / 2U : InExtent.Height;
    return { Width > 0U ? Width : 1U, Height > 0U ? Height : 1U, 1U };
}

Lumen::TExpected<Lumen::FRenderPipelineRef, Lumen::FRenderExtensionError>
Lumen::FLuminaPathTracerExtension::CreatePipeline ( IRenderFeatureHost &InHost,
                                                    const FString &InFragmentSource,
                                                    const USize InInlineConstantBytes,
                                                    FRenderBindingLayoutRef InBindingLayout,
                                                    const ERenderFormat InTargetFormat ) noexcept
{
    FRenderShaderDesc PixelDesc;
    PixelDesc.Stage      = ERenderShaderStage::Pixel;
    PixelDesc.Format     = ERenderShaderFormat::Source;
    PixelDesc.EntryPoint = "main";
    PixelDesc.Bytecode = TSpan<const Byte>( reinterpret_cast<const Byte *>( InFragmentSource.data() ), InFragmentSource.size() );
    PixelDesc.InlineConstantBytes = InInlineConstantBytes;
    LUMEN_EXPECT( PixelShader, InHost.CreateShader( PixelDesc ) );

    FRenderPipelineLayoutDesc LayoutDesc;
    LayoutDesc.BindingLayouts.push_back( InBindingLayout );
    LayoutDesc.InlineConstants.push_back(
        { .Offset = 0U, .Size = static_cast<UInt32>( InInlineConstantBytes ), .Stages = ERenderShaderStage::AllGraphics } );
    LUMEN_EXPECT( Layout, InHost.CreatePipelineLayout( LayoutDesc ) );

    FRenderGraphicsPipelineDesc PipelineDesc;
    PipelineDesc.VertexShader = VertexShader;
    PipelineDesc.PixelShader  = PixelShader;
    PipelineDesc.Layout       = Layout;
    PipelineDesc.Topology     = ERenderPrimitiveTopology::TriangleList;
    PipelineDesc.ColorFormats.push_back( InTargetFormat );
    return InHost.CreateGraphicsPipeline( PipelineDesc );
}

Lumen::FRenderExtensionExpected Lumen::FLuminaPathTracerExtension::Initialize ( IRenderFeatureHost &InHost,
                                                                                const FRenderExtent3D &InExtent ) noexcept
{
    Host             = &InHost;
    bSoftwareAdapter = InHost.GetCapabilities().bSoftwareAdapter;

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

    FRenderShaderDesc VertexDesc;
    VertexDesc.Stage      = ERenderShaderStage::Vertex;
    VertexDesc.Format     = ERenderShaderFormat::Source;
    VertexDesc.EntryPoint = "main";
    VertexDesc.Bytecode   = TSpan<const Byte>( reinterpret_cast<const Byte *>( VertexSource.data() ), VertexSource.size() );
    LUMEN_EXPECT( CreatedVertexShader, Host->CreateShader( VertexDesc ) );
    VertexShader = CreatedVertexShader;

    FRenderSamplerDesc SamplerDesc;
    SamplerDesc.MinFilter = ERenderFilter::Nearest;
    SamplerDesc.MagFilter = ERenderFilter::Nearest;
    SamplerDesc.MipFilter = ERenderFilter::Nearest;
    SamplerDesc.AddressU  = ERenderAddressMode::ClampToEdge;
    SamplerDesc.AddressV  = ERenderAddressMode::ClampToEdge;
    SamplerDesc.AddressW  = ERenderAddressMode::ClampToEdge;
    LUMEN_EXPECT( CreatedNearestSampler, Host->CreateSampler( SamplerDesc ) );
    NearestSampler        = CreatedNearestSampler;
    SamplerDesc.MinFilter = ERenderFilter::Linear;
    SamplerDesc.MagFilter = ERenderFilter::Linear;
    SamplerDesc.MipFilter = ERenderFilter::Linear;
    LUMEN_EXPECT( CreatedLinearSampler, Host->CreateSampler( SamplerDesc ) );
    LinearSampler = CreatedLinearSampler;

    FRenderBindingLayoutDesc OneTextureDesc;
    OneTextureDesc.Entries.push_back(
        { .Slot = 0U, .Type = ERenderBindingType::CombinedTextureSampler, .Stages = ERenderShaderStage::Pixel } );
    LUMEN_EXPECT( CreatedOneTextureLayout, Host->CreateBindingLayout( OneTextureDesc ) );
    OneTextureLayout                        = CreatedOneTextureLayout;
    FRenderBindingLayoutDesc TwoTextureDesc = OneTextureDesc;
    TwoTextureDesc.Entries.push_back(
        { .Slot = 1U, .Type = ERenderBindingType::CombinedTextureSampler, .Stages = ERenderShaderStage::Pixel } );
    LUMEN_EXPECT( CreatedTwoTextureLayout, Host->CreateBindingLayout( TwoTextureDesc ) );
    TwoTextureLayout = CreatedTwoTextureLayout;

    LUMEN_EXPECT( CreatedPathTrace, CreatePipeline( *Host, PathTraceSource, sizeof( FPathTraceConstants ), OneTextureLayout,
                                                    ERenderFormat::RGBA32Float ) );
    LUMEN_EXPECT( CreatedBloomExtract, CreatePipeline( *Host, BloomExtractSource, sizeof( FPostProcessConstants ),
                                                       OneTextureLayout, ERenderFormat::RGBA16Float ) );
    LUMEN_EXPECT( CreatedBloomBlur, CreatePipeline( *Host, BloomBlurSource, sizeof( FPostProcessConstants ), OneTextureLayout,
                                                    ERenderFormat::RGBA16Float ) );
    LUMEN_EXPECT( CreatedDisplay, CreatePipeline( *Host, DisplaySource, sizeof( FPostProcessConstants ), TwoTextureLayout,
                                                  ERenderFormat::RGBA8Unorm ) );
    PathTracePipeline    = CreatedPathTrace;
    BloomExtractPipeline = CreatedBloomExtract;
    BloomBlurPipeline    = CreatedBloomBlur;
    DisplayPipeline      = CreatedDisplay;

    FrameBuilder = Host->CreateFrameBuilder( FRenderFrameCapacities{ .PassCount              = 7U,
                                                                     .ResourceCount          = 5U,
                                                                     .ReadCountPerPass       = 2U,
                                                                     .WriteCountPerPass      = 1U,
                                                                     .DependencyCountPerPass = 7U,
                                                                     .TransitionCount        = 64U } );
    if ( not FrameBuilder.IsValid() )
    {
        return Lumen::MakeUnexpected( LuminaError( "Lumina could not create its frame builder." ) );
    }
    return CreateFrameResources( ScaleExtent( InExtent ) );
}

Lumen::FRenderExtensionExpected
Lumen::FLuminaPathTracerExtension::CreateFrameResources ( const FRenderExtent3D &InExtent ) noexcept
{
    AccumulationExtent = InExtent;
    BloomExtent = { InExtent.Width / 4U > 0U ? InExtent.Width / 4U : 1U, InExtent.Height / 4U > 0U ? InExtent.Height / 4U : 1U,
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
        LUMEN_EXPECT( AccumulationTexture,
                      Host->CreateTexture( { .Extent = AccumulationExtent, .Format = ERenderFormat::RGBA32Float } ) );
        LUMEN_EXPECT( AccumulationView, Host->CreateTextureView( AccumulationTexture ) );
        AccumulationTextures.push_back( AccumulationTexture );
        AccumulationViews.push_back( AccumulationView );

        LUMEN_EXPECT( BloomTexture, Host->CreateTexture( { .Extent = BloomExtent, .Format = ERenderFormat::RGBA16Float } ) );
        LUMEN_EXPECT( BloomView, Host->CreateTextureView( BloomTexture ) );
        BloomTextures.push_back( BloomTexture );
        BloomViews.push_back( BloomView );

        LUMEN_EXPECT( PathTraceBinding, Host->CreateBindingSet( OneTextureLayout ) );
        const FRenderBindingWrite PathTraceWrite{ .Slot = 0U, .View = AccumulationView, .Sampler = NearestSampler };
        LUMEN_EXPECT( PathTraceWriteResult,
                      Host->WriteBindingSet( PathTraceBinding, TSpan<const FRenderBindingWrite>( &PathTraceWrite, 1U ) ) );
        PathTraceBindings.push_back( PathTraceBinding );

        LUMEN_EXPECT( BloomExtractBinding, Host->CreateBindingSet( OneTextureLayout ) );
        const FRenderBindingWrite BloomExtractWrite{ .Slot = 0U, .View = AccumulationView, .Sampler = NearestSampler };
        LUMEN_EXPECT( BloomExtractWriteResult,
                      Host->WriteBindingSet( BloomExtractBinding, TSpan<const FRenderBindingWrite>( &BloomExtractWrite, 1U ) ) );
        BloomExtractBindings.push_back( BloomExtractBinding );

        LUMEN_EXPECT( DisplayBinding, Host->CreateBindingSet( TwoTextureLayout ) );
        const FRenderBindingWrite DisplayWrites[] = {
            { .Slot = 0U, .View = AccumulationView, .Sampler = NearestSampler },
            { .Slot = 1U, .View = BloomViews[0], .Sampler = LinearSampler },
        };
        LUMEN_EXPECT( DisplayWrite, Host->WriteBindingSet( DisplayBinding, DisplayWrites ) );
        DisplayBindings.push_back( DisplayBinding );
    }

    LUMEN_EXPECT( CreatedBlurHorizontal, Host->CreateBindingSet( OneTextureLayout ) );
    const FRenderBindingWrite BlurHorizontalWrite{ .Slot = 0U, .View = BloomViews[0], .Sampler = LinearSampler };
    LUMEN_EXPECT( BlurHorizontalWriteResult,
                  Host->WriteBindingSet( CreatedBlurHorizontal, TSpan<const FRenderBindingWrite>( &BlurHorizontalWrite, 1U ) ) );
    BlurHorizontalBinding = CreatedBlurHorizontal;
    LUMEN_EXPECT( CreatedBlurVertical, Host->CreateBindingSet( OneTextureLayout ) );
    const FRenderBindingWrite BlurVerticalWrite{ .Slot = 0U, .View = BloomViews[1], .Sampler = LinearSampler };
    LUMEN_EXPECT( BlurVerticalWriteResult,
                  Host->WriteBindingSet( CreatedBlurVertical, TSpan<const FRenderBindingWrite>( &BlurVerticalWrite, 1U ) ) );
    BlurVerticalBinding = CreatedBlurVertical;

    SampleCount = 0ULL;
    PingPong    = 0U;
    return {};
}

Lumen::FRenderExtensionExpected Lumen::FLuminaPathTracerExtension::Resize ( const FRenderExtent3D &InExtent ) noexcept
{
    const FRenderExtent3D Scaled = ScaleExtent( InExtent );
    if ( Scaled != AccumulationExtent )
    {
        LUMEN_TRY_EXPECT_ERROR( CreateFrameResources( Scaled ) );
    }
    SampleCount = 0ULL;
    PingPong    = 0U;
    return {};
}

Lumen::FRenderExtensionExpected
Lumen::FLuminaPathTracerExtension::Record ( const FRenderFrameExtensionContext &InContext ) noexcept
{
    if ( Host == nullptr or not FrameBuilder.IsValid() or AccumulationTextures.size() != 2U )
    {
        return Lumen::MakeUnexpected( LuminaError( "Lumina frame resources are not initialized." ) );
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
        return Lumen::MakeUnexpected( LuminaError( "The active Lumina camera has no transform." ) );
    }

    const Maths::FMat4x4f &WorldTransform = CameraTransform->WorldTransform;
    const Maths::FVec3f CameraPosition( WorldTransform[3].X, WorldTransform[3].Y, WorldTransform[3].Z );

    /** INFO: The shader basis is left-handed, so Right is the negation of the LookAt rig's column 0. */
    const Maths::FVec3f Forward( WorldTransform[2].X, WorldTransform[2].Y, WorldTransform[2].Z );
    const Maths::FVec3f Up( WorldTransform[1].X, WorldTransform[1].Y, WorldTransform[1].Z );
    const Maths::FVec3f Right( -WorldTransform[0].X, -WorldTransform[0].Y, -WorldTransform[0].Z );

    /** INFO: Perspective stores +/- 1 / tan(fov / 2) in [1][1]; only the magnitude is needed. */
    const Float32 ProjectionScale = ActiveCamera->Projection[1].Data[1];
    const Float32 TanHalfFov      = ProjectionScale != 0.0F ? 1.0F / Maths::Abs( ProjectionScale ) : 1.0F;

    /** INFO: A rotation-only look-at or a FOV change restarts accumulation exactly like a translation. */
    if ( not bHasCamera or CameraPosition != LastCameraPosition or Forward != LastCameraForward or TanHalfFov != LastTanHalfFov )
    {
        SampleCount = 0ULL;
        PingPong    = 0U;
    }
    bHasCamera                           = true;
    LastCameraPosition                   = CameraPosition;
    LastCameraForward                    = Forward;
    LastTanHalfFov                       = TanHalfFov;
    const UInt32 ReadIndex               = PingPong;
    const UInt32 WriteIndex              = 1U - PingPong;
    InContext.Status.ExtensionFrameCount = SampleCount;
    const FPathTraceConstants PathConstants{
        .ResolutionFrame =
            Maths::FVec4f( static_cast<Float32>( AccumulationExtent.Width ), static_cast<Float32>( AccumulationExtent.Height ),
                           static_cast<Float32>( SampleCount ), 0.0F ),
        .CameraPosition = Maths::FVec4f( CameraPosition.X, CameraPosition.Y, CameraPosition.Z, 0.0F ),
        .CameraForward  = Maths::FVec4f( Forward.X, Forward.Y, Forward.Z, 0.0F ),
        .CameraRight    = Maths::FVec4f( Right.X, Right.Y, Right.Z, 0.0F ),
        .CameraUp       = Maths::FVec4f( Up.X, Up.Y, Up.Z, 0.0F ),
        .CameraParams   = Maths::FVec4f( TanHalfFov, 0.0F, 0.0F, 0.0F ),
    };
    const FPostProcessConstants NextSampleConstants{
        .Values = Maths::FVec4f( static_cast<Float32>( SampleCount + 1ULL ), 0.0F, 0.0F, 0.0F ) };

    FrameBuilder->Reset();
    const auto Import = [this] ( const FRenderTextureRef &InTexture ) noexcept
    {
        return FrameBuilder->Import( InTexture );
    };
    LUMEN_EXPECT( AccumReadResource, Import( AccumulationTextures[ReadIndex] ) );
    LUMEN_EXPECT( AccumWriteResource, Import( AccumulationTextures[WriteIndex] ) );
    LUMEN_EXPECT( Bloom0Resource, Import( BloomTextures[0] ) );
    LUMEN_EXPECT( Bloom1Resource, Import( BloomTextures[1] ) );
    LUMEN_TRY_EXPECT_ERROR( GraphError( FrameBuilder->SetTarget( InContext.TargetTexture ) ) );
    LUMEN_EXPECT( TargetResource, Import( InContext.TargetTexture ) );

    TArray<FRenderFramePassHandle, 7U> Passes{};
    for ( FRenderFramePassHandle &Pass : Passes )
    {
        LUMEN_EXPECT( CreatedPass, FrameBuilder->AddPass( true ) );
        Pass = CreatedPass;
    }
    LUMEN_TRY_EXPECT_ERROR( GraphError( FrameBuilder->Read( Passes[0], AccumReadResource ) ) );
    LUMEN_TRY_EXPECT_ERROR( GraphError( FrameBuilder->Write( Passes[0], AccumWriteResource ) ) );
    LUMEN_TRY_EXPECT_ERROR( GraphError( FrameBuilder->Read( Passes[1], AccumWriteResource ) ) );
    LUMEN_TRY_EXPECT_ERROR( GraphError( FrameBuilder->Write( Passes[1], Bloom0Resource ) ) );
    for ( UInt32 BlurPass = 0U; BlurPass < 4U; ++BlurPass )
    {
        const Bool bHorizontal = ( BlurPass % 2U ) == 0U;
        LUMEN_TRY_EXPECT_ERROR(
            GraphError( FrameBuilder->Read( Passes[2U + BlurPass], bHorizontal ? Bloom0Resource : Bloom1Resource ) ) );
        LUMEN_TRY_EXPECT_ERROR(
            GraphError( FrameBuilder->Write( Passes[2U + BlurPass], bHorizontal ? Bloom1Resource : Bloom0Resource ) ) );
    }
    LUMEN_TRY_EXPECT_ERROR( GraphError( FrameBuilder->Read( Passes[6], AccumWriteResource ) ) );
    LUMEN_TRY_EXPECT_ERROR( GraphError( FrameBuilder->Read( Passes[6], Bloom0Resource ) ) );
    LUMEN_TRY_EXPECT_ERROR( GraphError( FrameBuilder->Write( Passes[6], TargetResource ) ) );

    TUniquePtr<IRenderCompiledFrame> Compiled = FrameBuilder->Compile();
    if ( not Compiled.IsValid() )
    {
        return Lumen::MakeUnexpected( LuminaError( "Lumina render graph compilation failed." ) );
    }

    for ( UInt32 PassOrdinal = 0U; PassOrdinal < 7U; ++PassOrdinal )
    {
        FRenderExtensionExpected Recorded{};
        switch ( PassOrdinal )
        {
        case 0U:
            Recorded = Compiled->RecordPass(
                InContext.Encoder, PassOrdinal, PathTracePipeline, PathTraceBindings[ReadIndex], AccumulationViews[WriteIndex],
                AccumulationExtent,
                TSpan<const Byte>( reinterpret_cast<const Byte *>( &PathConstants ), sizeof( PathConstants ) ) );
            break;
        case 1U:
            Recorded = Compiled->RecordPass(
                InContext.Encoder, PassOrdinal, BloomExtractPipeline, BloomExtractBindings[WriteIndex], BloomViews[0],
                BloomExtent,
                TSpan<const Byte>( reinterpret_cast<const Byte *>( &NextSampleConstants ), sizeof( NextSampleConstants ) ) );
            break;
        case 2U:
        case 4U:
        {
            const Float32 Scale = PassOrdinal == 2U ? 1.0F : 2.2F;
            const FPostProcessConstants Constants{ .Values = Maths::FVec4f( Scale, 0.0F, 0.0F, 0.0F ) };
            Recorded = Compiled->RecordPass(
                InContext.Encoder, PassOrdinal, BloomBlurPipeline, BlurHorizontalBinding, BloomViews[1], BloomExtent,
                TSpan<const Byte>( reinterpret_cast<const Byte *>( &Constants ), sizeof( Constants ) ) );
            break;
        }
        case 3U:
        case 5U:
        {
            const Float32 Scale = PassOrdinal == 3U ? 1.0F : 2.2F;
            const FPostProcessConstants Constants{ .Values = Maths::FVec4f( 0.0F, Scale, 0.0F, 0.0F ) };
            Recorded = Compiled->RecordPass(
                InContext.Encoder, PassOrdinal, BloomBlurPipeline, BlurVerticalBinding, BloomViews[0], BloomExtent,
                TSpan<const Byte>( reinterpret_cast<const Byte *>( &Constants ), sizeof( Constants ) ) );
            break;
        }
        case 6U:
        {
            const FPostProcessConstants Constants{ .Values =
                                                       Maths::FVec4f( static_cast<Float32>( SampleCount + 1ULL ),
                                                                      static_cast<Float32>( InContext.Extent.Width ),
                                                                      static_cast<Float32>( InContext.Extent.Height ), 0.0F ) };
            Recorded = Compiled->RecordPass(
                InContext.Encoder, PassOrdinal, DisplayPipeline, DisplayBindings[WriteIndex], InContext.TargetView,
                InContext.Extent, TSpan<const Byte>( reinterpret_cast<const Byte *>( &Constants ), sizeof( Constants ) ) );
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
    return {};
}

void Lumen::FLuminaPathTracerExtension::Commit ( const UInt64 LUMEN_UNUSED InTimelineValue,
                                                 FRenderFrameStatus &InOutStatus ) noexcept
{
    PingPong = 1U - PingPong;
    ++SampleCount;
    InOutStatus.ExtensionFrameCount = SampleCount;
}
