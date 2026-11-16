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
        auto Allocator = *Device.CreateCommandAllocator( ERHIQueueType::Compute );
        auto List      = *Device.CreateCommandList( Allocator );
        auto Layout    = *Device.CreatePipelineLayout( {} );
        FRHIShaderDesc ShaderDesc;
        ShaderDesc.Stage = ERHIShaderStage::Compute;
        auto Shader      = *Device.CreateShader( ShaderDesc );
        auto Pipeline    = *Device.CreateComputePipeline( { .Shader = Shader, .Layout = Layout } );
        auto Arguments   = *Device.CreateBuffer( { .Size = 64ULL, .Usage = ERHIResourceUsage::IndirectArgument } );
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
