/**
 * @file RHIPipelinesConformanceTests.cpp
 * @brief Runs portable graphics and compute pipeline creation.
 */

#include <Device/IRHIDevice.hpp>
#include <Device/RHIConformanceDeviceFactory.hpp>

#include <Pipelines/IRHIPipeline.hpp>
#include <Pipelines/RHIComputePipelineDesc.hpp>
#include <Pipelines/RHIGraphicsPipelineDesc.hpp>

#include <Shaders/IRHIShader.hpp>
#include <Tests/UnitTest.hpp>

namespace Lumen
{
namespace
{

    namespace
    {

        TSharedRef<IRHIShader> CreateShader ( IRHIDevice &Device, const ERHIShaderStage Stage )
        {
            auto Shader = Tests::GetRHIConformanceDeviceFactory().CreateShader( Device, Stage );
            LUMEN_CHECK_TRUE( Shader.HasValue() );
            return *Shader;
        }

    } // namespace

    LUMEN_TEST( RHIPipelinesConformance, CreatesGraphicsPipelineWithCanonicalCompatibility )
    {
        auto Device = Tests::GetRHIConformanceDeviceFactory().CreateDevice();
        LUMEN_CHECK_TRUE( Device.HasValue() );
        FRHIBindingLayoutDesc BindingDesc;
        BindingDesc.Entries.push_back( { .Slot = 0U, .Stages = ERHIShaderStage::AllGraphics } );
        auto BindingLayout = ( *Device )->CreateBindingLayout( BindingDesc );
        LUMEN_CHECK_TRUE( BindingLayout.HasValue() );
        FRHIPipelineLayoutDesc LayoutDesc;
        LayoutDesc.BindingLayouts.push_back( *BindingLayout );
        auto Layout = ( *Device )->CreatePipelineLayout( LayoutDesc );
        LUMEN_CHECK_TRUE( Layout.HasValue() );
        FRHIGraphicsPipelineDesc Desc;
        Desc.VertexShader       = CreateShader( **Device, ERHIShaderStage::Vertex );
        Desc.PixelShader        = CreateShader( **Device, ERHIShaderStage::Pixel );
        Desc.Layout             = *Layout;
        Desc.ColorFormats       = { ERHIFormat::RGBA8Unorm };
        Desc.DepthStencilFormat = ERHIFormat::D24UnormS8Uint;
        Desc.Multisample.Count  = ERHISampleCount::Count4;
        Desc.VertexLayout.Buffers.push_back( { .Slot = 0U, .Stride = 12U, .Rate = ERHIVertexInputRate::Vertex } );
        Desc.VertexLayout.Attributes.push_back(
            { .Location = 0U, .BufferSlot = 0U, .Format = ERHIFormat::RGB32Float, .Offset = 0U } );
        auto Pipeline = ( *Device )->CreateGraphicsPipeline( Desc );
        LUMEN_CHECK_TRUE( Pipeline.HasValue() );
        LUMEN_CHECK_EQ( ( *Pipeline )->GetType(), ERHIPipelineType::Graphics );
        const auto &Compatibility = ( *Pipeline )->GetCompatibility();
        LUMEN_CHECK_EQ( Compatibility.ColorFormats, Desc.ColorFormats );
        LUMEN_CHECK_EQ( Compatibility.DepthStencilFormat, Desc.DepthStencilFormat );
        LUMEN_CHECK_EQ( Compatibility.SampleCount, Desc.Multisample.Count );
        LUMEN_CHECK_EQ( Compatibility.VertexLayoutHash, Desc.VertexLayout.GetHash() );
        LUMEN_CHECK_EQ( Compatibility.LayoutHash, RHIHashCombine( 0ULL, BindingDesc.GetHash() ) );
    }
    LUMEN_TEST( RHIPipelinesConformance, CreatesComputePipelineAndRejectsInvalidDescriptors )
    {
        auto Device = Tests::GetRHIConformanceDeviceFactory().CreateDevice();
        LUMEN_CHECK_TRUE( Device.HasValue() );
        auto Layout = ( *Device )->CreatePipelineLayout( {} );
        LUMEN_CHECK_TRUE( Layout.HasValue() );
        FRHIComputePipelineDesc Desc;
        Desc.Shader   = CreateShader( **Device, ERHIShaderStage::Compute );
        Desc.Layout   = *Layout;
        auto Pipeline = ( *Device )->CreateComputePipeline( Desc );
        LUMEN_CHECK_TRUE( Pipeline.HasValue() );
        LUMEN_CHECK_EQ( ( *Pipeline )->GetType(), ERHIPipelineType::Compute );
        Desc.Shader           = CreateShader( **Device, ERHIShaderStage::Vertex );
        const auto WrongStage = ( *Device )->CreateComputePipeline( Desc );
        LUMEN_CHECK_HAS_ERROR( WrongStage );
        LUMEN_CHECK_EQ( WrongStage.Error().Code, ERHIErrorCode::InvalidArgument );
        LUMEN_CHECK_EQ( WrongStage.Error().Condition, ERHIErrorCondition::Pipeline );
        LUMEN_CHECK_HAS_ERROR( ( *Device )->CreateGraphicsPipeline( {} ) );
    }

    LUMEN_TEST( RHIPipelinesConformance, EquivalentLayoutsHaveStableCompatibilityAndDistinctIdentities )
    {
        auto Device = Tests::GetRHIConformanceDeviceFactory().CreateDevice();
        LUMEN_CHECK_TRUE( Device.HasValue() );
        FRHIBindingLayoutDesc BindingDesc;
        BindingDesc.Entries.push_back( { .Slot = 0U, .Type = ERHIBindingType::ReadOnlyBuffer } );
        auto FirstBindingLayout  = ( *Device )->CreateBindingLayout( BindingDesc );
        auto SecondBindingLayout = ( *Device )->CreateBindingLayout( BindingDesc );
        LUMEN_CHECK_TRUE( FirstBindingLayout.HasValue() );
        LUMEN_CHECK_TRUE( SecondBindingLayout.HasValue() );

        FRHIPipelineLayoutDesc FirstLayoutDesc;
        FirstLayoutDesc.BindingLayouts.push_back( *FirstBindingLayout );
        FRHIPipelineLayoutDesc SecondLayoutDesc;
        SecondLayoutDesc.BindingLayouts.push_back( *SecondBindingLayout );
        auto FirstLayout  = ( *Device )->CreatePipelineLayout( FirstLayoutDesc );
        auto SecondLayout = ( *Device )->CreatePipelineLayout( SecondLayoutDesc );
        LUMEN_CHECK_TRUE( FirstLayout.HasValue() );
        LUMEN_CHECK_TRUE( SecondLayout.HasValue() );

        FRHIComputePipelineDesc FirstDesc;
        FirstDesc.Shader = CreateShader( **Device, ERHIShaderStage::Compute );
        FirstDesc.Layout = *FirstLayout;
        FRHIComputePipelineDesc SecondDesc;
        SecondDesc.Shader   = CreateShader( **Device, ERHIShaderStage::Compute );
        SecondDesc.Layout   = *SecondLayout;
        auto FirstPipeline  = ( *Device )->CreateComputePipeline( FirstDesc );
        auto SecondPipeline = ( *Device )->CreateComputePipeline( SecondDesc );
        LUMEN_CHECK_TRUE( FirstPipeline.HasValue() );
        LUMEN_CHECK_TRUE( SecondPipeline.HasValue() );
        LUMEN_CHECK_NOT_EQ( ( *FirstPipeline )->GetId().GetValue(), ( *SecondPipeline )->GetId().GetValue() );
        LUMEN_CHECK_TRUE( ( *FirstPipeline )->GetCompatibility() == ( *SecondPipeline )->GetCompatibility() );
        LUMEN_CHECK_EQ( ( *FirstPipeline )->GetCompatibility().GetHash(), ( *SecondPipeline )->GetCompatibility().GetHash() );
    }

} // namespace
} // namespace Lumen
