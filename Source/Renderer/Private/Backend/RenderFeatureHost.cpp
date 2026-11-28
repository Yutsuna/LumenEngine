/**
 * @file RenderFeatureHost.cpp
 * @brief Neutral-to-RHI adapter implementation.
 */

#include <Backend/RenderFeatureHost.hpp>

#include <Algo/ForEach.hpp>
#include <Templates/Unexpected.hpp>

#include <RHI/Bindings/IRHIBindingLayout.hpp>
#include <RHI/Bindings/IRHIBindingSet.hpp>
#include <RHI/Bindings/RHIBindingLayoutDesc.hpp>
#include <RHI/Bindings/RHIBindingWrite.hpp>
#include <RHI/Commands/IRHICommandAllocator.hpp>
#include <RHI/Commands/RHIDrawArguments.hpp>
#include <RHI/Commands/RHITransition.hpp>
#include <RHI/Pipelines/IRHIPipelineLayout.hpp>
#include <RHI/Pipelines/RHIGraphicsPipelineDesc.hpp>
#include <RHI/Pipelines/RHIPipelineLayoutDesc.hpp>
#include <RHI/Resources/IRHISampler.hpp>
#include <RHI/Resources/IRHITexture.hpp>
#include <RHI/Resources/IRHITextureView.hpp>
#include <RHI/Resources/RHIResourceUsage.hpp>
#include <RHI/Resources/RHISamplerDesc.hpp>
#include <RHI/Resources/RHITextureDesc.hpp>
#include <RHI/Resources/RHITextureViewDesc.hpp>
#include <RHI/Shaders/IRHIShader.hpp>
#include <RHI/Shaders/RHIShaderDesc.hpp>
#include <RHI/Types/RHIFormat.hpp>
#include <RHI/Types/RHIResourceState.hpp>

namespace
{

[[nodiscard]] Lumen::RHI::ERHIFormat ToRHIFormat ( const Lumen::ERenderFormat InFormat ) noexcept
{
    using namespace Lumen;
    switch ( InFormat )
    {
    case ERenderFormat::RGBA8Unorm:
        return RHI::ERHIFormat::RGBA8Unorm;
    case ERenderFormat::RGBA16Float:
        return RHI::ERHIFormat::RGBA16Float;
    case ERenderFormat::RGBA32Float:
        return RHI::ERHIFormat::RGBA32Float;
    case ERenderFormat::RGB32Float:
        return RHI::ERHIFormat::RGB32Float;
    case ERenderFormat::Unknown:
    default:
        return RHI::ERHIFormat::Unknown;
    }
}

[[nodiscard]] Lumen::RHI::ERHIShaderFormat ToRHIShaderFormat ( const Lumen::ERenderShaderFormat InFormat ) noexcept
{
    using namespace Lumen;
    switch ( InFormat )
    {
    case ERenderShaderFormat::Source:
        return RHI::ERHIShaderFormat::Source;
    case ERenderShaderFormat::Bytecode:
        return RHI::ERHIShaderFormat::PortableBytecode;
    case ERenderShaderFormat::Null:
    default:
        return RHI::ERHIShaderFormat::Null;
    }
}

[[nodiscard]] Lumen::RHI::ERHIShaderStage ToRHIShaderStage ( const Lumen::ERenderShaderStage InStage ) noexcept
{
    using namespace Lumen;
    switch ( InStage )
    {
    case ERenderShaderStage::Vertex:
        return RHI::ERHIShaderStage::Vertex;
    case ERenderShaderStage::Pixel:
        return RHI::ERHIShaderStage::Pixel;
    case ERenderShaderStage::AllGraphics:
    default:
        return RHI::ERHIShaderStage::AllGraphics;
    }
}

[[nodiscard]] Lumen::RHI::ERHIShaderBindingKind ToRHIBindingKind ( const Lumen::ERenderShaderBindingKind InKind ) noexcept
{
    using namespace Lumen;
    switch ( InKind )
    {
    case ERenderShaderBindingKind::ConstantBuffer:
    default:
        return RHI::ERHIShaderBindingKind::ConstantBuffer;
    }
}

[[nodiscard]] Lumen::RHI::ERHISamplerFilter ToRHIFilter ( const Lumen::ERenderFilter InFilter ) noexcept
{
    using namespace Lumen;
    switch ( InFilter )
    {
    case ERenderFilter::Nearest:
        return RHI::ERHISamplerFilter::Nearest;
    case ERenderFilter::Linear:
    default:
        return RHI::ERHISamplerFilter::Linear;
    }
}

[[nodiscard]] Lumen::RHI::ERHISamplerAddressMode ToRHIAddress ( const Lumen::ERenderAddressMode InMode ) noexcept
{
    using namespace Lumen;
    switch ( InMode )
    {
    case ERenderAddressMode::ClampToEdge:
    default:
        return RHI::ERHISamplerAddressMode::ClampToEdge;
    }
}

[[nodiscard]] Lumen::RHI::ERHIBindingType ToRHIBindingType ( const Lumen::ERenderBindingType InType ) noexcept
{
    using namespace Lumen;
    switch ( InType )
    {
    case ERenderBindingType::CombinedTextureSampler:
    default:
        return RHI::ERHIBindingType::CombinedTextureSampler;
    }
}

[[nodiscard]] Lumen::RHI::ERHIPrimitiveTopology ToRHITopology ( const Lumen::ERenderPrimitiveTopology InTopology ) noexcept
{
    using namespace Lumen;
    switch ( InTopology )
    {
    case ERenderPrimitiveTopology::TriangleList:
    default:
        return RHI::ERHIPrimitiveTopology::TriangleList;
    }
}

[[nodiscard]] Lumen::FRenderExtensionError FeatureError ( const Lumen::FStringView InMessage ) noexcept
{
    return { Lumen::ERenderExtensionErrorCode::CreationFailed, Lumen::FString( InMessage ) };
}

[[nodiscard]] Lumen::FRenderExtensionError RecordError ( const Lumen::FStringView InMessage ) noexcept
{
    return { Lumen::ERenderExtensionErrorCode::RecordFailed, Lumen::FString( InMessage ) };
}

} // namespace

Lumen::FRenderFeatureHost::FRenderFeatureHost ( TSharedRef<RHI::IRHIDevice> InDevice,
                                                const Bool bInSoftwareAdapter,
                                                FString InDeviceName ) noexcept
    : Device( InDevice )
{
    Capabilities.bSoftwareAdapter = bInSoftwareAdapter;
    Capabilities.DeviceName       = std::move( InDeviceName );
}

const Lumen::FRenderHostCapabilities &Lumen::FRenderFeatureHost::GetCapabilities () const noexcept
{
    return Capabilities;
}

Lumen::TRenderExtensionExpected<Lumen::FRenderShaderRef>
Lumen::FRenderFeatureHost::CreateShader ( const FRenderShaderDesc &InDesc ) noexcept
{
    RHI::FRHIShaderDesc Desc;
    Desc.Stage                          = ToRHIShaderStage( InDesc.Stage );
    Desc.Format                         = ToRHIShaderFormat( InDesc.Format );
    Desc.EntryPoint                     = InDesc.EntryPoint;
    Desc.Bytecode                       = InDesc.Bytecode;
    Desc.Reflection.InlineConstantBytes = static_cast<UInt32>( InDesc.InlineConstantBytes );
    if ( InDesc.Stage == ERenderShaderStage::Pixel )
    {
        Desc.Reflection.Bindings.push_back(
            { .Slot = 0U, .Kind = ToRHIBindingKind( InDesc.BindingKind ), .Stages = RHI::ERHIShaderStage::Pixel } );
    }
    auto Result = Device->CreateShader( Desc );
    if ( not Result )
    {
        return MakeUnexpected( FeatureError( Result.Error().Message ) );
    }
    return MakeShared<FRenderShaderObject>( FRenderShaderObject{ *Result } );
}

Lumen::TRenderExtensionExpected<Lumen::FRenderSamplerRef>
Lumen::FRenderFeatureHost::CreateSampler ( const FRenderSamplerDesc &InDesc ) noexcept
{
    RHI::FRHISamplerDesc Desc;
    Desc.MinFilter = ToRHIFilter( InDesc.MinFilter );
    Desc.MagFilter = ToRHIFilter( InDesc.MagFilter );
    Desc.MipFilter = ToRHIFilter( InDesc.MipFilter );
    Desc.AddressU  = ToRHIAddress( InDesc.AddressU );
    Desc.AddressV  = ToRHIAddress( InDesc.AddressV );
    Desc.AddressW  = ToRHIAddress( InDesc.AddressW );
    auto Result    = Device->CreateSampler( Desc );
    if ( not Result )
    {
        return MakeUnexpected( FeatureError( Result.Error().Message ) );
    }
    return MakeShared<FRenderSamplerObject>( FRenderSamplerObject{ *Result } );
}

Lumen::TRenderExtensionExpected<Lumen::FRenderBindingLayoutRef>
Lumen::FRenderFeatureHost::CreateBindingLayout ( const FRenderBindingLayoutDesc &InDesc ) noexcept
{
    RHI::FRHIBindingLayoutDesc Desc;
    for ( const FRenderBindingLayoutDesc::FEntry &Entry : InDesc.Entries )
    {
        Desc.Entries.push_back(
            { .Slot = Entry.Slot, .Type = ToRHIBindingType( Entry.Type ), .Stages = ToRHIShaderStage( Entry.Stages ) } );
    }
    auto Result = Device->CreateBindingLayout( Desc );
    if ( not Result )
    {
        return MakeUnexpected( FeatureError( Result.Error().Message ) );
    }
    return MakeShared<FRenderBindingLayoutObject>( FRenderBindingLayoutObject{ *Result } );
}

Lumen::TRenderExtensionExpected<Lumen::FRenderPipelineLayoutRef>
Lumen::FRenderFeatureHost::CreatePipelineLayout ( const FRenderPipelineLayoutDesc &InDesc ) noexcept
{
    RHI::FRHIPipelineLayoutDesc Desc;
    for ( const FRenderBindingLayoutRef &LayoutRef : InDesc.BindingLayouts )
    {
        const FRenderBindingLayoutObject *const Layout = AsOpaque<FRenderBindingLayoutObject>( LayoutRef );
        if ( Layout == nullptr )
        {
            return MakeUnexpected( FeatureError( "Invalid binding layout in pipeline layout." ) );
        }
        Desc.BindingLayouts.push_back( Layout->Layout );
    }
    for ( const FRenderPipelineLayoutDesc::FRange &Range : InDesc.InlineConstants )
    {
        Desc.InlineConstants.push_back(
            { .Offset = Range.Offset, .Size = Range.Size, .Stages = ToRHIShaderStage( Range.Stages ) } );
    }
    auto Result = Device->CreatePipelineLayout( Desc );
    if ( not Result )
    {
        return MakeUnexpected( FeatureError( Result.Error().Message ) );
    }
    return MakeShared<FRenderPipelineLayoutObject>( FRenderPipelineLayoutObject{ *Result } );
}

Lumen::TRenderExtensionExpected<Lumen::FRenderPipelineRef>
Lumen::FRenderFeatureHost::CreateGraphicsPipeline ( const FRenderGraphicsPipelineDesc &InDesc ) noexcept
{
    const FRenderShaderObject *const Vertex         = AsOpaque<FRenderShaderObject>( InDesc.VertexShader );
    const FRenderShaderObject *const Pixel          = AsOpaque<FRenderShaderObject>( InDesc.PixelShader );
    const FRenderPipelineLayoutObject *const Layout = AsOpaque<FRenderPipelineLayoutObject>( InDesc.Layout );
    if ( Vertex == nullptr or Pixel == nullptr or Layout == nullptr )
    {
        return MakeUnexpected( FeatureError( "Invalid pipeline descriptor." ) );
    }

    RHI::FRHIGraphicsPipelineDesc Desc;
    Desc.VertexShader = Vertex->Shader;
    Desc.PixelShader  = Pixel->Shader;
    Desc.Layout       = Layout->Layout;
    Desc.Topology     = ToRHITopology( InDesc.Topology );
    for ( const ERenderFormat Format : InDesc.ColorFormats )
    {
        Desc.ColorFormats.push_back( ToRHIFormat( Format ) );
    }
    auto Result = Device->CreateGraphicsPipeline( Desc );
    if ( not Result )
    {
        return MakeUnexpected( FeatureError( Result.Error().Message ) );
    }
    return MakeShared<FRenderPipelineObject>( FRenderPipelineObject{ *Result } );
}

Lumen::TRenderExtensionExpected<Lumen::FRenderTextureRef>
Lumen::FRenderFeatureHost::CreateTexture ( const FRenderTextureDesc &InDesc ) noexcept
{
    RHI::FRHITextureDesc Desc;
    Desc.Extent       = { InDesc.Extent.Width, InDesc.Extent.Height, InDesc.Extent.Depth };
    Desc.Format       = ToRHIFormat( InDesc.Format );
    Desc.Usage        = RHI::ERHIResourceUsage::RenderTarget | RHI::ERHIResourceUsage::ShaderResource;
    Desc.InitialState = RHI::ERHIResourceState::Common;
    auto Result       = Device->CreateTexture( Desc );
    if ( not Result )
    {
        return MakeUnexpected( FeatureError( Result.Error().Message ) );
    }
    return MakeShared<FRenderTextureObject>( FRenderTextureObject{ *Result } );
}

Lumen::TRenderExtensionExpected<Lumen::FRenderTextureViewRef>
Lumen::FRenderFeatureHost::CreateTextureView ( const FRenderTextureRef &InTexture ) noexcept
{
    FRenderTextureObject *const Texture = AsOpaque<FRenderTextureObject>( InTexture );
    if ( Texture == nullptr )
    {
        return MakeUnexpected( FeatureError( "Invalid texture in view creation." ) );
    }
    RHI::FRHITextureViewDesc Desc;
    Desc.Texture = Texture->Texture;
    auto Result  = Device->CreateTextureView( Desc );
    if ( not Result )
    {
        return MakeUnexpected( FeatureError( Result.Error().Message ) );
    }
    return MakeShared<FRenderTextureViewObject>(
        FRenderTextureViewObject{ *Result, StaticCastSharedPtr<FRenderTextureObject>( InTexture ) } );
}

Lumen::TRenderExtensionExpected<Lumen::FRenderBindingSetRef>
Lumen::FRenderFeatureHost::CreateBindingSet ( const FRenderBindingLayoutRef &InLayout ) noexcept
{
    const FRenderBindingLayoutObject *const Layout = AsOpaque<FRenderBindingLayoutObject>( InLayout );
    if ( Layout == nullptr )
    {
        return MakeUnexpected( FeatureError( "Invalid binding layout in binding set creation." ) );
    }
    auto Result = Device->CreateBindingSet( Layout->Layout );
    if ( not Result )
    {
        return MakeUnexpected( FeatureError( Result.Error().Message ) );
    }
    return MakeShared<FRenderBindingSetObject>( FRenderBindingSetObject{ *Result } );
}

Lumen::FRenderExtensionExpected Lumen::FRenderFeatureHost::WriteBindingSet ( const FRenderBindingSetRef &InSet,
                                                                             TSpan<const FRenderBindingWrite> InWrites ) noexcept
{
    FRenderBindingSetObject *const SetObject = AsOpaque<FRenderBindingSetObject>( InSet );
    if ( SetObject == nullptr )
    {
        return MakeUnexpected( FeatureError( "Invalid binding set in write." ) );
    }
    TVector<RHI::FRHIBindingWrite> Writes;
    Writes.reserve( InWrites.size() );
    for ( const FRenderBindingWrite &Write : InWrites )
    {
        const FRenderTextureViewObject *const View = AsOpaque<FRenderTextureViewObject>( Write.View );
        const FRenderSamplerObject *const Sampler  = AsOpaque<FRenderSamplerObject>( Write.Sampler );
        if ( View == nullptr or Sampler == nullptr )
        {
            return MakeUnexpected( FeatureError( "Invalid combined texture/sampler binding." ) );
        }
        Writes.push_back(
            { .Slot     = Write.Slot,
              .Resource = RHI::FRHICombinedTextureSamplerBinding{ .View = View->View, .Sampler = Sampler->Sampler } } );
    }
    const RHI::FRHIExpected Result = SetObject->Set->Write( Writes );
    if ( not Result )
    {
        return MakeUnexpected( FeatureError( Result.Error().Message ) );
    }
    return {};
}

Lumen::TPair<Lumen::FRenderTextureRef, Lumen::FRenderTextureViewRef>
Lumen::FRenderFeatureHost::WrapExternalTexture ( TSharedRef<RHI::IRHITexture> InTexture,
                                                 TSharedRef<RHI::IRHITextureView> InView ) noexcept
{
    TSharedRef<FRenderTextureObject> Texture =
        MakeShared<FRenderTextureObject>( FRenderTextureObject{ InTexture, InTexture->GetInitialState() } );
    TSharedRef<FRenderTextureViewObject> View =
        MakeShared<FRenderTextureViewObject>( FRenderTextureViewObject{ InView, TSharedPtr<FRenderTextureObject>( Texture ) } );
    return { Texture, View };
}

Lumen::TUniquePtr<Lumen::IRenderFrameBuilder>
Lumen::FRenderFeatureHost::CreateFrameBuilder ( const FRenderFrameCapacities &InCapacities ) noexcept
{
    return MakeUnique<FRenderFrameBuilderAdapter>( *this, InCapacities );
}

void Lumen::FRenderFeatureHost::QueueState ( TSharedPtr<FRenderTextureObject> InTexture,
                                             const RHI::ERHIResourceState InState ) noexcept
{
    if ( InTexture.IsValid() )
    {
        PendingStates.push_back( { InTexture, InState } );
    }
}

void Lumen::FRenderFeatureHost::CommitStates () noexcept
{
    for ( const FRenderPendingState &Pending : PendingStates )
    {
        Pending.Texture->CommittedState = Pending.State;
    }
    PendingStates.clear();
}

void Lumen::FRenderFeatureHost::DiscardStates () noexcept
{
    PendingStates.clear();
}

Lumen::FRenderPassEncoderAdapter::FRenderPassEncoderAdapter ( RHI::IRHICommandList &InCommandList ) noexcept
    : CommandList( InCommandList )
{
}

Lumen::FRenderExtensionExpected Lumen::FRenderPassEncoderAdapter::SetPipeline ( const FRenderPipelineRef &InPipeline ) noexcept
{
    const FRenderPipelineObject *const Object = AsOpaque<FRenderPipelineObject>( InPipeline );
    if ( Object == nullptr )
    {
        return MakeUnexpected( RecordError( "Invalid pipeline in pass encoder." ) );
    }
    const RHI::FRHIExpected Result = CommandList.SetPipeline( Object->Pipeline );
    return Result ? FRenderExtensionExpected{} : MakeUnexpected( RecordError( Result.Error().Message ) );
}

Lumen::FRenderExtensionExpected Lumen::FRenderPassEncoderAdapter::BindSet ( const UInt32 InSlot,
                                                                            const FRenderBindingSetRef &InSet ) noexcept
{
    const FRenderBindingSetObject *const Object = AsOpaque<FRenderBindingSetObject>( InSet );
    if ( Object == nullptr )
    {
        return MakeUnexpected( RecordError( "Invalid binding set in pass encoder." ) );
    }
    const RHI::FRHIExpected Result = CommandList.SetBindingSet( InSlot, Object->Set );
    return Result ? FRenderExtensionExpected{} : MakeUnexpected( RecordError( Result.Error().Message ) );
}

Lumen::FRenderExtensionExpected Lumen::FRenderPassEncoderAdapter::SetConstants ( const ERenderShaderStage InStage,
                                                                                 const UInt32 InOffset,
                                                                                 const TSpan<const Byte> InConstants ) noexcept
{
    const RHI::FRHIExpected Result = CommandList.SetInlineConstants( ToRHIShaderStage( InStage ), InOffset, InConstants );
    return Result ? FRenderExtensionExpected{} : MakeUnexpected( RecordError( Result.Error().Message ) );
}

Lumen::FRenderExtensionExpected Lumen::FRenderPassEncoderAdapter::Draw ( const UInt32 InVertexCount ) noexcept
{
    const RHI::FRHIExpected Result = CommandList.Draw( { .VertexCount = InVertexCount } );
    return Result ? FRenderExtensionExpected{} : MakeUnexpected( RecordError( Result.Error().Message ) );
}

Lumen::FRenderFrameBuilderAdapter::FRenderFrameBuilderAdapter ( FRenderFeatureHost &InHost,
                                                                const FRenderFrameCapacities &InCapacities ) noexcept
    : Host( InHost )
{
    Graph.Emplace( FRenderGraphCapacities{ .PassCount              = InCapacities.PassCount,
                                           .ResourceCount          = InCapacities.ResourceCount,
                                           .ReadCountPerPass       = InCapacities.ReadCountPerPass,
                                           .WriteCountPerPass      = InCapacities.WriteCountPerPass,
                                           .DependencyCountPerPass = InCapacities.DependencyCountPerPass,
                                           .TransitionCount        = InCapacities.TransitionCount } );
}

void Lumen::FRenderFrameBuilderAdapter::Reset () noexcept
{
    Graph->Reset();
    Target.Reset();
    LastImported.Reset();
    Imports.clear();
}

Lumen::TRenderExtensionExpected<Lumen::FRenderFrameResourceHandle>
Lumen::FRenderFrameBuilderAdapter::Import ( const FRenderTextureRef &InTexture ) noexcept
{
    FRenderTextureObject *const Object = AsOpaque<FRenderTextureObject>( InTexture );
    if ( Object == nullptr )
    {
        return MakeUnexpected( FeatureError( "Invalid imported texture." ) );
    }
    const RHI::ERHIResourceState Final =
        Object == Target.Get() ? RHI::ERHIResourceState::Present : RHI::ERHIResourceState::ShaderResource;
    const FRenderImportedResourceDesc Desc{ .InitialState = Object->CommittedState,
                                            .FinalState   = Final,
                                            .InitialQueue = RHI::ERHIQueueType::Graphics,
                                            .FinalQueue   = RHI::ERHIQueueType::Graphics };
    auto Result = Graph->Import( Object->Texture, Desc );
    if ( not Result )
    {
        return MakeUnexpected( FeatureError( "Render graph rejected a texture import." ) );
    }
    LastImported = StaticCastSharedPtr<FRenderTextureObject>( InTexture );
    Imports.push_back( LastImported );
    return TRenderExtensionExpected<FRenderFrameResourceHandle>{
        FRenderFrameResourceHandle{ .Index = Result->Index, .Generation = Result->Generation } };
}

Lumen::FRenderExtensionExpected Lumen::FRenderFrameBuilderAdapter::SetTarget ( const FRenderTextureRef &InTexture ) noexcept
{
    Target = StaticCastSharedPtr<FRenderTextureObject>( InTexture );
    return Target.IsValid() ? FRenderExtensionExpected{} : MakeUnexpected( FeatureError( "Invalid target texture." ) );
}

Lumen::TRenderExtensionExpected<Lumen::FRenderFramePassHandle>
Lumen::FRenderFrameBuilderAdapter::AddPass ( const Bool bHasSideEffects ) noexcept
{
    auto Result = Graph->AddPass( { .Queue = RHI::ERHIQueueType::Graphics, .bHasSideEffects = bHasSideEffects } );
    if ( not Result )
    {
        return MakeUnexpected( FeatureError( "Render graph rejected a pass." ) );
    }
    return TRenderExtensionExpected<FRenderFramePassHandle>{ FRenderFramePassHandle{ .Ordinal = Result->Ordinal } };
}

Lumen::FRenderExtensionExpected Lumen::FRenderFrameBuilderAdapter::Read ( const FRenderFramePassHandle InPass,
                                                                          const FRenderFrameResourceHandle InResource ) noexcept
{
    const auto Result = Graph->Read( FRenderPassHandle{ .Ordinal = InPass.Ordinal },
                                     FRenderResourceHandle{ .Index = InResource.Index, .Generation = InResource.Generation },
                                     RHI::ERHIResourceState::ShaderResource );
    return Result ? FRenderExtensionExpected{} : MakeUnexpected( FeatureError( "Render graph rejected a read." ) );
}

Lumen::FRenderExtensionExpected Lumen::FRenderFrameBuilderAdapter::Write ( const FRenderFramePassHandle InPass,
                                                                           const FRenderFrameResourceHandle InResource ) noexcept
{
    const auto Result = Graph->Write( FRenderPassHandle{ .Ordinal = InPass.Ordinal },
                                      FRenderResourceHandle{ .Index = InResource.Index, .Generation = InResource.Generation },
                                      RHI::ERHIResourceState::RenderTarget );
    return Result ? FRenderExtensionExpected{} : MakeUnexpected( FeatureError( "Render graph rejected a write." ) );
}

Lumen::TUniquePtr<Lumen::IRenderCompiledFrame> Lumen::FRenderFrameBuilderAdapter::Compile () noexcept
{
    auto Compiled = std::move( *Graph ).Compile();
    if ( not Compiled )
    {
        return nullptr;
    }
    TVector<FRenderTransition> Transitions;
    for ( const FRenderTransition &Transition : Compiled->GetTransitions() )
    {
        Transitions.push_back( Transition );
    }
    return MakeUnique<FRenderCompiledFrameAdapter>( Host, Imports, Transitions, FRenderGraphContext( std::move( *Compiled ) ) );
}

Lumen::FRenderCompiledFrameAdapter::FRenderCompiledFrameAdapter ( FRenderFeatureHost &InHost,
                                                                  TVector<TSharedPtr<FRenderTextureObject>> InImports,
                                                                  const TSpan<const FRenderTransition> InTransitions,
                                                                  FRenderGraphContext &&InContext ) noexcept
    : Host( InHost ), Imports( std::move( InImports ) ), Context( std::move( InContext ) )
{
    for ( const FRenderTransition &Transition : InTransitions )
    {
        Transitions.push_back( Transition );
    }
}

Lumen::Bool Lumen::FRenderCompiledFrameAdapter::RecordTransitions ( RHI::IRHICommandList &InCommandList,
                                                                    const ERenderTransitionRecordPoint InPoint,
                                                                    const UInt32 InPassOrdinal ) noexcept
{
    for ( USize Index = 0U; Index < Transitions.size(); ++Index )
    {
        if ( Transitions[Index].RecordPoint != InPoint or Transitions[Index].RecordPassOrdinal != InPassOrdinal )
        {
            continue;
        }
        const auto Resolved = Context.ResolveTransition( Index );
        if ( not Resolved )
        {
            return false;
        }
        const RHI::FRHITransition &Values = *Resolved;
        if ( not InCommandList.Transition( TSpan<const RHI::FRHITransition>( &Values, 1U ) ) )
        {
            return false;
        }
    }
    return true;
}

Lumen::FRenderExtensionExpected Lumen::FRenderCompiledFrameAdapter::RecordPass ( IRenderPassEncoder &InEncoder,
                                                                                 const UInt32 InPassOrdinal,
                                                                                 const FRenderPipelineRef &InPipeline,
                                                                                 const FRenderBindingSetRef &InBindingSet,
                                                                                 const FRenderTextureViewRef &InTargetView,
                                                                                 const FRenderExtent3D &InTargetExtent,
                                                                                 const TSpan<const Byte> InConstants ) noexcept
{
    FRenderPassEncoderAdapter *const Adapter               = dynamic_cast<FRenderPassEncoderAdapter *>( &InEncoder );
    const FRenderTextureViewObject *const TargetViewObject = AsOpaque<FRenderTextureViewObject>( InTargetView );
    if ( Adapter == nullptr or TargetViewObject == nullptr )
    {
        return MakeUnexpected( RecordError( "Unsupported pass encoder or target view." ) );
    }
    RHI::IRHICommandList &CommandList = Adapter->GetCommandList();

    if ( not RecordTransitions( CommandList, ERenderTransitionRecordPoint::BeforePass, InPassOrdinal ) )
    {
        return MakeUnexpected( RecordError( "Failed to record pre-pass transitions." ) );
    }

    RHI::FRHIRenderingDesc RenderingDesc;
    RenderingDesc.Area = RHI::FRHIRect2D{ 0, 0, InTargetExtent.Width, InTargetExtent.Height };
    RHI::FRHIRenderingAttachment Attachment;
    Attachment.View    = TargetViewObject->View;
    Attachment.LoadOp  = RHI::ERHIRenderingLoadOp::DontCare;
    Attachment.StoreOp = RHI::ERHIRenderingStoreOp::Store;
    RenderingDesc.ColorAttachments.push_back( Attachment );
    if ( not CommandList.BeginRendering( RenderingDesc ).HasValue() )
    {
        return MakeUnexpected( RecordError( "Failed to begin rendering." ) );
    }

    LUMEN_EXPECT( PipelineResult, Adapter->SetPipeline( InPipeline ) );
    LUMEN_EXPECT( SetResult, Adapter->BindSet( 0U, InBindingSet ) );
    LUMEN_EXPECT( ConstantsResult, Adapter->SetConstants( ERenderShaderStage::Pixel, 0U, InConstants ) );
    LUMEN_EXPECT( DrawResult, Adapter->Draw( 3U ) );
    if ( not CommandList.EndRendering().HasValue() or
         not RecordTransitions( CommandList, ERenderTransitionRecordPoint::AfterPass, InPassOrdinal ) )
    {
        return MakeUnexpected( RecordError( "Failed to end rendering or record post-pass transitions." ) );
    }

    for ( const TSharedPtr<FRenderTextureObject> &Texture : Imports )
    {
        const RHI::ERHIResourceState Final =
            Texture == TargetViewObject->Texture ? RHI::ERHIResourceState::Present : RHI::ERHIResourceState::ShaderResource;
        Host.QueueState( Texture, Final );
    }
    return {};
}
