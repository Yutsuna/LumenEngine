/**
 * @file RHIComputePipelineDescTests.cpp
 * @brief Tests compute pipeline descriptor requirements.
 */

#include <Pipelines/NullPipelines.hpp>
#include <RHI/Pipelines/RHIComputePipelineDesc.hpp>

#include <Shaders/NullShaders.hpp>
#include <Tests/UnitTest.hpp>

namespace Lumen
{

namespace RHI
{
    namespace
    {

        LUMEN_TEST( RHIComputePipelineDesc, RequiresComputeShaderAndLayout )
        {
            FRHIShaderDesc ShaderDesc;
            ShaderDesc.Stage = ERHIShaderStage::Compute;
            FRHIComputePipelineDesc Desc;
            Desc.Shader = StaticCastSharedRef<IRHIShader>( MakeShared<Tests::FNullShader>( ShaderDesc, FRHIObjectId{ 1ULL } ) );
            Desc.Layout = StaticCastSharedRef<IRHIPipelineLayout>(
                MakeShared<Tests::FNullPipelineLayout>( FRHIPipelineLayoutDesc{}, FRHIObjectId{ 2ULL } ) );
            LUMEN_CHECK_TRUE( Desc.Validate().HasValue() );
            Desc.Layout.Reset();
            LUMEN_CHECK_HAS_ERROR( Desc.Validate() );
            Desc.Layout = StaticCastSharedRef<IRHIPipelineLayout>(
                MakeShared<Tests::FNullPipelineLayout>( FRHIPipelineLayoutDesc{}, FRHIObjectId{ 2ULL } ) );
            Desc.Shader.Reset();
            LUMEN_CHECK_HAS_ERROR( Desc.Validate() );
        }

        LUMEN_TEST( RHIComputePipelineDesc, RejectsNonComputeShader )
        {
            FRHIShaderDesc ShaderDesc;
            ShaderDesc.Stage = ERHIShaderStage::Vertex;
            FRHIComputePipelineDesc Desc;
            Desc.Shader = StaticCastSharedRef<IRHIShader>( MakeShared<Tests::FNullShader>( ShaderDesc, FRHIObjectId{ 1ULL } ) );
            Desc.Layout = StaticCastSharedRef<IRHIPipelineLayout>(
                MakeShared<Tests::FNullPipelineLayout>( FRHIPipelineLayoutDesc{}, FRHIObjectId{ 2ULL } ) );
            LUMEN_CHECK_HAS_ERROR( Desc.Validate() );
        }

    } // namespace
} // namespace RHI

} // namespace Lumen
