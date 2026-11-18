/**
 * @file RHICommandsTierBTests.cpp
 * @brief Tests dynamic states, indirect work, and illegal nesting.
 */

#include "Device/NullDevice.hpp"
#include <Commands/IRHICommandAllocator.hpp>
#include <Commands/IRHICommandList.hpp>
#include <Pipelines/IRHIPipeline.hpp>
#include <Pipelines/RHIComputePipelineDesc.hpp>
#include <Pipelines/RHIPipelineLayoutDesc.hpp>
#include <Shaders/RHIShaderDesc.hpp>
#include <Tests/UnitTest.hpp>

namespace Lumen
{

namespace
{
    LUMEN_TEST( RHICommandsTierB, RejectsInvalidNestingAndRecordsIndirectDispatch )
    {
        Tests::FNullDevice Device;
        TSharedRef<IRHICommandAllocator> Allocator = *Device.CreateCommandAllocator( ERHIQueueType::Compute );
        TSharedRef<IRHICommandList> List           = *Device.CreateCommandList( Allocator );
        TSharedRef<IRHIPipelineLayout> Layout      = *Device.CreatePipelineLayout( {} );
        FRHIShaderDesc ShaderDesc;
        ShaderDesc.Stage                  = ERHIShaderStage::Compute;
        TSharedRef<IRHIShader> Shader     = *Device.CreateShader( ShaderDesc );
        TSharedRef<IRHIPipeline> Pipeline = *Device.CreateComputePipeline( { .Shader = Shader, .Layout = Layout } );
        TSharedRef<IRHIBuffer> Arguments =
            *Device.CreateBuffer( { .Size = 64ULL, .Usage = ERHIResourceUsage::IndirectArgument } );
        LUMEN_CHECK_TRUE( List->Begin().HasValue() );
        LUMEN_CHECK_TRUE( List->SetPipeline( Pipeline ).HasValue() );
        LUMEN_CHECK_TRUE( List->DispatchIndirect( Arguments, { .Count = 2U, .Stride = 16U } ).HasValue() );
        LUMEN_CHECK_HAS_ERROR( List->BeginRendering( {} ) );
        LUMEN_CHECK_HAS_ERROR( List->EndRendering() );
        LUMEN_CHECK_TRUE( List->End().HasValue() );
        LUMEN_CHECK_HAS_ERROR( List->Dispatch( {} ) );
    }

} // namespace

} // namespace Lumen
