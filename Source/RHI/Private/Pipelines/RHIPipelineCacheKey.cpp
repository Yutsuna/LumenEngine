/**
 * @file RHIPipelineCacheKey.cpp
 * @brief Implements canonical pipeline and shader cache hashing.
 */

#include <RHI/Pipelines/RHIPipelineCacheKey.hpp>

namespace
{

template <typename ValueType> void HashField ( Lumen::UInt64 &InOutHash, const ValueType &InValue ) noexcept
{
    InOutHash = Lumen::RHI::RHIHashCombine( InOutHash, Lumen::RHI::RHIHashValue( InValue ) );
}

Lumen::UInt64 HashLayout ( const Lumen::RHI::IRHIPipelineLayout *InLayout ) noexcept
{
    using namespace Lumen;
    using namespace Lumen::RHI;
    if ( InLayout == nullptr )
    {
        return 0ULL;
    }
    UInt64 Hash                        = 0ULL;
    const FRHIPipelineLayoutDesc &Desc = InLayout->GetDesc();
    HashField( Hash, static_cast<UInt64>( Desc.BindingLayouts.size() ) );
    for ( const TSharedRef<IRHIBindingLayout> &Layout : Desc.BindingLayouts )
    {
        HashField( Hash, Layout->GetDesc().GetHash() );
    }
    HashField( Hash, static_cast<UInt64>( Desc.InlineConstants.size() ) );
    for ( const FRHIInlineConstantRange &Range : Desc.InlineConstants )
    {
        HashField( Hash, Range.Offset );
        HashField( Hash, Range.Size );
        HashField( Hash, Range.Stages );
    }
    return Hash;
}

Lumen::UInt64 HashStencil ( Lumen::UInt64 InHash, const Lumen::RHI::FRHIStencilState &InStencil ) noexcept
{
    HashField( InHash, InStencil.FailOp );
    HashField( InHash, InStencil.DepthFailOp );
    HashField( InHash, InStencil.PassOp );
    HashField( InHash, InStencil.Compare );
    return InHash;
}

Lumen::UInt64 HashGraphicsState ( const Lumen::RHI::FRHIGraphicsPipelineDesc &InDesc ) noexcept
{
    using namespace Lumen;
    using namespace Lumen::RHI;
    UInt64 Hash = HashLayout( InDesc.Layout.Get() );
    HashField( Hash, InDesc.VertexLayout.GetHash() );
    HashField( Hash, InDesc.Topology );

    HashField( Hash, static_cast<UInt64>( InDesc.Blend.Attachments.size() ) );
    for ( const FRHIBlendAttachmentState &Attachment : InDesc.Blend.Attachments )
    {
        HashField( Hash, Attachment.bEnabled );
        HashField( Hash, Attachment.SourceColor );
        HashField( Hash, Attachment.DestinationColor );
        HashField( Hash, Attachment.ColorOp );
        HashField( Hash, Attachment.SourceAlpha );
        HashField( Hash, Attachment.DestinationAlpha );
        HashField( Hash, Attachment.AlphaOp );
        HashField( Hash, Attachment.WriteMask );
    }

    HashField( Hash, InDesc.DepthStencil.bDepthTest );
    HashField( Hash, InDesc.DepthStencil.bDepthWrite );
    HashField( Hash, InDesc.DepthStencil.bStencilTest );
    HashField( Hash, InDesc.DepthStencil.DepthCompare );
    Hash = HashStencil( Hash, InDesc.DepthStencil.Front );
    Hash = HashStencil( Hash, InDesc.DepthStencil.Back );

    HashField( Hash, InDesc.Rasterizer.Fill );
    HashField( Hash, InDesc.Rasterizer.Cull );
    HashField( Hash, InDesc.Rasterizer.FrontFace );
    HashField( Hash, InDesc.Rasterizer.bDepthClip );
    HashField( Hash, InDesc.Rasterizer.DepthBiasConstant );
    HashField( Hash, InDesc.Rasterizer.DepthBiasSlope );

    HashField( Hash, InDesc.Multisample.Count );
    HashField( Hash, InDesc.Multisample.SampleMask );
    HashField( Hash, InDesc.Multisample.bAlphaToCoverage );
    HashField( Hash, InDesc.DynamicStates );
    HashField( Hash, static_cast<UInt64>( InDesc.ColorFormats.size() ) );
    for ( const ERHIFormat Format : InDesc.ColorFormats )
    {
        HashField( Hash, Format );
    }
    HashField( Hash, InDesc.DepthStencilFormat );
    return Hash;
}

Lumen::RHI::FRHIPipelineCompatibility MakeCompatibility ( const Lumen::RHI::IRHIPipelineLayout *InLayout ) noexcept
{
    Lumen::RHI::FRHIPipelineCompatibility Compatibility;
    if ( InLayout != nullptr )
    {
        for ( const auto &Layout : InLayout->GetDesc().BindingLayouts )
        {
            Compatibility.LayoutHash = Lumen::RHI::RHIHashCombine( Compatibility.LayoutHash, Layout->GetDesc().GetHash() );
        }
    }
    return Compatibility;
}

} // namespace

Lumen::UInt64 Lumen::RHI::FRHIPipelineCacheKey::GetHash () const noexcept
{
    UInt64 Hash = Compatibility.GetHash();
    HashField( Hash, StateHash );
    HashField( Hash, static_cast<UInt64>( ShaderHashes.size() ) );
    for ( const UInt64 ShaderHash : ShaderHashes )
    {
        HashField( Hash, ShaderHash );
    }
    HashField( Hash, BackendCacheSchemaVersion );
    return Hash;
}

Lumen::UInt64 Lumen::RHI::GetRHIShaderCacheHash ( const IRHIShader &InShader ) noexcept
{
    const FRHIShaderDesc &Desc = InShader.GetDesc();
    UInt64 Hash                = RHIHashBytes( Desc.Bytecode );
    HashField( Hash, Desc.Stage );
    HashField( Hash, Desc.Format );
    const Byte *EntryPointBytes = reinterpret_cast<const Byte *>( Desc.EntryPoint.data() );
    HashField( Hash, RHIHashBytes( TSpan<const Byte>( EntryPointBytes, Desc.EntryPoint.size() ) ) );
    HashField( Hash, static_cast<UInt64>( Desc.Reflection.Bindings.size() ) );
    for ( const FRHIShaderBindingReflection &Binding : Desc.Reflection.Bindings )
    {
        HashField( Hash, Binding.Slot );
        HashField( Hash, Binding.Kind );
        HashField( Hash, Binding.Count );
        HashField( Hash, Binding.Stages );
    }
    for ( const UInt32 Size : Desc.Reflection.WorkgroupSize )
    {
        HashField( Hash, Size );
    }
    HashField( Hash, Desc.Reflection.InlineConstantBytes );
    return Hash;
}

Lumen::RHI::FRHIPipelineCacheKey Lumen::RHI::MakeRHIGraphicsPipelineCacheKey ( const FRHIGraphicsPipelineDesc &InDesc,
                                                                               const UInt64 InBackendCacheSchemaVersion ) noexcept
{
    FRHIPipelineCacheKey Key;
    Key.Compatibility                    = MakeCompatibility( InDesc.Layout.Get() );
    Key.Compatibility.ColorFormats       = InDesc.ColorFormats;
    Key.Compatibility.DepthStencilFormat = InDesc.DepthStencilFormat;
    Key.Compatibility.SampleCount        = InDesc.Multisample.Count;
    Key.Compatibility.VertexLayoutHash   = InDesc.VertexLayout.GetHash();
    Key.StateHash                        = HashGraphicsState( InDesc );
    Key.BackendCacheSchemaVersion        = InBackendCacheSchemaVersion;
    if ( InDesc.VertexShader )
    {
        Key.ShaderHashes.push_back( GetRHIShaderCacheHash( *InDesc.VertexShader ) );
    }
    if ( InDesc.PixelShader )
    {
        Key.ShaderHashes.push_back( GetRHIShaderCacheHash( *InDesc.PixelShader ) );
    }
    return Key;
}

Lumen::RHI::FRHIPipelineCacheKey Lumen::RHI::MakeRHIComputePipelineCacheKey ( const FRHIComputePipelineDesc &InDesc,
                                                                              const UInt64 InBackendCacheSchemaVersion ) noexcept
{
    FRHIPipelineCacheKey Key;
    Key.Compatibility             = MakeCompatibility( InDesc.Layout.Get() );
    Key.StateHash                 = HashLayout( InDesc.Layout.Get() );
    Key.BackendCacheSchemaVersion = InBackendCacheSchemaVersion;
    if ( InDesc.Shader )
    {
        Key.ShaderHashes.push_back( GetRHIShaderCacheHash( *InDesc.Shader ) );
    }
    return Key;
}
