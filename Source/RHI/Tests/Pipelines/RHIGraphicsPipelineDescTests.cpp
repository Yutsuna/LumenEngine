/**
 * @file RHIGraphicsPipelineDescTests.cpp
 * @brief Tests complete graphics pipeline validation.
 */

#include <Device/NullDevice.hpp>
#include <RHI/Pipelines/RHIGraphicsPipelineDesc.hpp>
#include <RHI/Pipelines/RHIPipelineLayoutDesc.hpp>

#include <RHI/Shaders/RHIShaderDesc.hpp>

#include <Tests/UnitTest.hpp>

namespace Lumen
{

namespace RHI
{
    namespace
    {

        namespace
        {

            TSharedRef<IRHIShader> MakeShader ( Tests::FNullDevice &Device, const ERHIShaderStage Stage )
            {
                FRHIShaderDesc Desc;
                Desc.Stage                                          = Stage;
                TExpected<TSharedRef<IRHIShader>, FRHIError> Shader = Device.CreateShader( Desc );
                LUMEN_CHECK_TRUE( Shader.HasValue() );
                return *Shader;
            }
            FRHIGraphicsPipelineDesc MakeValidDesc ()
            {
                Tests::FNullDevice Device;
                FRHIGraphicsPipelineDesc Desc;
                Desc.VertexShader = MakeShader( Device, ERHIShaderStage::Vertex );
                Desc.PixelShader  = MakeShader( Device, ERHIShaderStage::Pixel );
                Desc.Layout       = *Device.CreatePipelineLayout( {} );
                Desc.ColorFormats = { ERHIFormat::RGBA8Unorm };
                return Desc;
            }
        } // namespace
        LUMEN_TEST( RHIGraphicsPipelineDesc, RequiresCorrectShaderStagesAndLayout )
        {
            FRHIGraphicsPipelineDesc Desc = MakeValidDesc();
            LUMEN_CHECK_TRUE( Desc.Validate().HasValue() );
            Tests::FNullDevice Device;
            Desc.VertexShader = MakeShader( Device, ERHIShaderStage::Pixel );
            LUMEN_CHECK_HAS_ERROR( Desc.Validate() );
            Desc = MakeValidDesc();
            Desc.PixelShader.Reset();
            LUMEN_CHECK_HAS_ERROR( Desc.Validate() );
            Desc = MakeValidDesc();
            Desc.Layout.Reset();
            LUMEN_CHECK_HAS_ERROR( Desc.Validate() );
        }
        LUMEN_TEST( RHIGraphicsPipelineDesc, RejectsInvalidNestedStateAndAttachmentFormats )
        {
            FRHIGraphicsPipelineDesc Desc = MakeValidDesc();
            Desc.Multisample.SampleMask   = 0U;
            LUMEN_CHECK_HAS_ERROR( Desc.Validate() );
            Desc = MakeValidDesc();
            Desc.ColorFormats.push_back( ERHIFormat::Unknown );
            LUMEN_CHECK_HAS_ERROR( Desc.Validate() );
            Desc                 = MakeValidDesc();
            Desc.ColorFormats[0] = ERHIFormat::D32Float;
            LUMEN_CHECK_HAS_ERROR( Desc.Validate() );
            Desc                    = MakeValidDesc();
            Desc.DepthStencilFormat = ERHIFormat::RGBA8Unorm;
            LUMEN_CHECK_HAS_ERROR( Desc.Validate() );
            Desc          = MakeValidDesc();
            Desc.Topology = static_cast<ERHIPrimitiveTopology>( 6U );
            LUMEN_CHECK_HAS_ERROR( Desc.Validate() );
            Desc               = MakeValidDesc();
            Desc.DynamicStates = static_cast<ERHIDynamicState>( 1U << 31U );
            LUMEN_CHECK_HAS_ERROR( Desc.Validate() );
        }

    } // namespace
} // namespace RHI

} // namespace Lumen
