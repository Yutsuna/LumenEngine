/**
 * @file RHILimitsTests.cpp
 * @brief Tests quantitative limit validation.
 */

#include <RHI/Types/RHILimits.hpp>
#include <Tests/UnitTest.hpp>

namespace Lumen
{

namespace RHI
{

    LUMEN_TEST( RHILimits, RejectsZeroAndInvalidAlignment )
    {
        LUMEN_CHECK_HAS_ERROR( FRHILimits{}.Validate() );
        FRHILimits Limits{
            .MaxBufferSize           = 1024ULL,
            .MaxTextureDimension1D   = 1024U,
            .MaxTextureDimension2D   = 1024U,
            .MaxTextureDimension3D   = 256U,
            .MaxTextureArrayLayers   = 256U,
            .MaxColorAttachments     = 8U,
            .MaxBindingSets          = 8U,
            .MaxDescriptorsPerSet    = 1024U,
            .MaxVertexAttributes     = 32U,
            .MaxVertexBuffers        = 16U,
            .MaxComputeWorkgroupSize = 1024U,
        };

        LUMEN_CHECK_TRUE( Limits.Validate().HasValue() );
        Limits.MinBufferAlignment = 3ULL;
        LUMEN_CHECK_HAS_ERROR( Limits.Validate() );
    }

} // namespace RHI

} // namespace Lumen
