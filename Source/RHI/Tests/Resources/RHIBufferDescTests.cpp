/**
 * @file RHIBufferDescTests.cpp
 * @brief Tests buffer descriptor validation.
 */

#include <RHI/Resources/RHIBufferDesc.hpp>
#include <Tests/UnitTest.hpp>

namespace Lumen
{

namespace RHI
{

    LUMEN_TEST( RHIBufferDesc, RequiresSizeAndKnownUsage )
    {
        FRHIBufferDesc Desc;
        Desc.Size  = 64ULL;
        Desc.Usage = ERHIResourceUsage::VertexBuffer;
        LUMEN_CHECK_TRUE( Desc.Validate().HasValue() );
        LUMEN_CHECK_HAS_ERROR( FRHIBufferDesc{}.Validate() );
        Desc.Usage = static_cast<ERHIResourceUsage>( 1U << 31U );
        LUMEN_CHECK_HAS_ERROR( Desc.Validate() );
    }

} // namespace RHI

} // namespace Lumen
