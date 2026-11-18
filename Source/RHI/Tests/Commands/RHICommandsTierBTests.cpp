/**
 * @file RHICommandsTierBTests.cpp
 * @brief Tests dynamic states, indirect work, and illegal nesting.
 */

#include "Device/NullDevice.hpp"
#include <RHI/Commands/IRHICommandAllocator.hpp>
#include <RHI/Commands/IRHICommandList.hpp>
#include <RHI/Pipelines/IRHIPipeline.hpp>
#include <RHI/Pipelines/RHIComputePipelineDesc.hpp>
#include <RHI/Pipelines/RHIPipelineLayoutDesc.hpp>
#include <RHI/Shaders/RHIShaderDesc.hpp>
#include <Tests/UnitTest.hpp>

namespace Lumen
{

namespace RHI
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

} // namespace RHI

} // namespace Lumen
