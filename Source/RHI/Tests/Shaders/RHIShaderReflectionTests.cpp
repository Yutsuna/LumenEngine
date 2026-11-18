/**
 * @file RHIShaderReflectionTests.cpp
 * @brief Tests shader reflection invariants.
 */

#include <RHI/Shaders/RHIShaderReflection.hpp>
#include <Tests/UnitTest.hpp>

namespace Lumen
{

namespace RHI
{
    namespace
    {

        LUMEN_TEST( RHIShaderReflection, RejectsZeroCountsAndDuplicateSlots )
        {
            FRHIShaderReflection Reflection;
            Reflection.Bindings.push_back( { .Slot = 0U, .Stages = ERHIShaderStage::Vertex } );
            Reflection.Bindings.push_back( { .Slot = 3U, .Stages = ERHIShaderStage::Pixel } );
            LUMEN_CHECK_TRUE( Reflection.Validate().HasValue() );
            Reflection.Bindings[1].Count = 0U;
            LUMEN_CHECK_HAS_ERROR( Reflection.Validate() );
            Reflection.Bindings[1].Count = 1U;
            Reflection.Bindings[1].Slot  = 0U;
            LUMEN_CHECK_HAS_ERROR( Reflection.Validate() );
        }

        LUMEN_TEST( RHIShaderReflection, EmptyReflectionAndComputeMetadataAreValid )
        {
            FRHIShaderReflection Reflection;
            Reflection.WorkgroupSize       = { 8U, 4U, 2U };
            Reflection.InlineConstantBytes = 16U;
            LUMEN_CHECK_TRUE( Reflection.Validate().HasValue() );
            LUMEN_CHECK_EQ( Reflection.WorkgroupSize[1], 4U );
        }

    } // namespace
} // namespace RHI

} // namespace Lumen
