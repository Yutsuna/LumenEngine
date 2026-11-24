/**
 * @file RHIGraphicsPipelineDesc.cpp
 * @brief Implements graphics pipeline validation.
 */

#include <RHI/Pipelines/RHIGraphicsPipelineDesc.hpp>
#include <RHI/Types/RHIFormatInfo.hpp>
Lumen::RHI::FRHIExpected Lumen::RHI::FRHIGraphicsPipelineDesc::Validate () const noexcept
{
    if ( not VertexShader or not PixelShader or not Layout or VertexShader->GetDesc().Stage != ERHIShaderStage::Vertex or
         PixelShader->GetDesc().Stage != ERHIShaderStage::Pixel or not VertexLayout.Validate() or not Blend.Validate() or
         not DepthStencil.Validate() or not Rasterizer.Validate() or not Multisample.Validate() or
         Topology > ERHIPrimitiveTopology::PatchList or ( static_cast<UInt32>( DynamicStates ) & ~0x1FU ) != 0U )
    {
        return TUnexpected<FRHIError>( MakeRHIInvalidPipeline( "Invalid graphics pipeline" ) );
    }
    for ( const ERHIFormat Format : ColorFormats )
    {
        const TOptional<FRHIFormatInfo> Info = TryGetRHIFormatInfo( Format );
        if ( not Info or not Info->HasColor() )
        {
            return TUnexpected<FRHIError>( MakeRHIInvalidPipeline( "Invalid color attachment format" ) );
        }
    }
    if ( DepthStencilFormat != ERHIFormat::Unknown )
    {
        const TOptional<FRHIFormatInfo> Info = TryGetRHIFormatInfo( DepthStencilFormat );
        if ( not Info or ( not Info->HasDepth() and not Info->HasStencil() ) )
        {
            return TUnexpected<FRHIError>( MakeRHIInvalidPipeline( "Invalid depth-stencil attachment format" ) );
        }
    }
    return {};
}
