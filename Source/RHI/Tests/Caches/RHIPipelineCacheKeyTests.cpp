/**
 * @file RHIPipelineCacheKeyTests.cpp
 * @brief Tests canonical pipeline state and shader cache hashes.
 */

#include <Device/NullDevice.hpp>
#include <RHI/Pipelines/RHIPipelineCacheKey.hpp>
#include <Templates/Array.hpp>
#include <Tests/UnitTest.hpp>

namespace Lumen
{

namespace RHI
{
    namespace
    {
        LUMEN_TEST( RHIPipelineCacheKey, ComputeKeysIncludeLayoutAndShaderContent )
        {
            Tests::FNullDevice Device;
            TArray<Byte, 1U> Bytecode = { Byte{ 1U } };
            FRHIShaderDesc ShaderDesc;
            ShaderDesc.Stage                           = ERHIShaderStage::Compute;
            ShaderDesc.Bytecode                        = Bytecode;
            TSharedRef<IRHIShader> FirstShader         = *Device.CreateShader( ShaderDesc );
            Bytecode[0]                                = Byte{ 2U };
            TSharedRef<IRHIShader> SecondShader        = *Device.CreateShader( ShaderDesc );
            TSharedRef<IRHIPipelineLayout> FirstLayout = *Device.CreatePipelineLayout( {} );

            FRHIComputePipelineDesc FirstDesc;
            FirstDesc.Shader                     = FirstShader;
            FirstDesc.Layout                     = FirstLayout;
            const FRHIPipelineCacheKey FirstKey  = MakeRHIComputePipelineCacheKey( FirstDesc, 1ULL );
            FRHIComputePipelineDesc SecondDesc   = FirstDesc;
            SecondDesc.Shader                    = SecondShader;
            const FRHIPipelineCacheKey SecondKey = MakeRHIComputePipelineCacheKey( SecondDesc, 1ULL );
            LUMEN_CHECK_TRUE( FirstKey.Compatibility == SecondKey.Compatibility );
            LUMEN_CHECK_EQ( FirstKey.StateHash, SecondKey.StateHash );
            LUMEN_CHECK_NOT_EQ( FirstKey.ShaderHashes[0], SecondKey.ShaderHashes[0] );

            FRHIPipelineLayoutDesc LayoutDesc;
            LayoutDesc.InlineConstants.push_back( { .Offset = 0U, .Size = 4U, .Stages = ERHIShaderStage::Compute } );
            TSharedRef<IRHIPipelineLayout> SecondLayout = *Device.CreatePipelineLayout( LayoutDesc );
            FRHIComputePipelineDesc ThirdDesc           = FirstDesc;
            ThirdDesc.Layout                            = SecondLayout;
            const FRHIPipelineCacheKey ThirdKey         = MakeRHIComputePipelineCacheKey( ThirdDesc, 1ULL );
            LUMEN_CHECK_TRUE( FirstKey.Compatibility == ThirdKey.Compatibility );
            LUMEN_CHECK_NOT_EQ( FirstKey.StateHash, ThirdKey.StateHash );
            LUMEN_CHECK_TRUE( FirstKey.ShaderHashes == ThirdKey.ShaderHashes );
        }

        LUMEN_TEST( RHIPipelineCacheKey, GraphicsKeysIncludeImmutableStateAndOrderedShaders )
        {
            Tests::FNullDevice Device;
            FRHIShaderDesc ShaderDesc;
            ShaderDesc.Stage                      = ERHIShaderStage::Vertex;
            TSharedRef<IRHIShader> VertexShader   = *Device.CreateShader( ShaderDesc );
            ShaderDesc.Stage                      = ERHIShaderStage::Pixel;
            TSharedRef<IRHIShader> PixelShader    = *Device.CreateShader( ShaderDesc );
            TSharedRef<IRHIPipelineLayout> Layout = *Device.CreatePipelineLayout( {} );

            FRHIGraphicsPipelineDesc Desc;
            Desc.VertexShader = VertexShader;
            Desc.PixelShader  = PixelShader;
            Desc.Layout       = Layout;
            Desc.ColorFormats = { ERHIFormat::RGBA8Unorm };
            LUMEN_CHECK_TRUE( Desc.Validate().HasValue() );
            const FRHIPipelineCacheKey FirstKey = MakeRHIGraphicsPipelineCacheKey( Desc, 1ULL );
            LUMEN_CHECK_EQ( FirstKey.ShaderHashes.size(), 2U );

            Desc.Rasterizer.Fill                  = ERHIFillMode::Wireframe;
            const FRHIPipelineCacheKey OtherState = MakeRHIGraphicsPipelineCacheKey( Desc, 1ULL );
            LUMEN_CHECK_TRUE( FirstKey.Compatibility == OtherState.Compatibility );
            LUMEN_CHECK_NOT_EQ( FirstKey.StateHash, OtherState.StateHash );
            LUMEN_CHECK_TRUE( FirstKey.ShaderHashes == OtherState.ShaderHashes );

            TArray<Byte, 1U> PixelBytes            = { Byte{ 7U } };
            ShaderDesc.Bytecode                    = PixelBytes;
            TSharedRef<IRHIShader> OtherPixel      = *Device.CreateShader( ShaderDesc );
            Desc.PixelShader                       = OtherPixel;
            const FRHIPipelineCacheKey OtherShader = MakeRHIGraphicsPipelineCacheKey( Desc, 1ULL );
            LUMEN_CHECK_EQ( FirstKey.ShaderHashes[0], OtherShader.ShaderHashes[0] );
            LUMEN_CHECK_NOT_EQ( FirstKey.ShaderHashes[1], OtherShader.ShaderHashes[1] );
        }
    } // namespace
} // namespace RHI

} // namespace Lumen
