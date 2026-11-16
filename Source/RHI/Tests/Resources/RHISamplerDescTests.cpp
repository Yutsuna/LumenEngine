/**
 * @file RHISamplerDescTests.cpp
 * @brief Tests sampler validation.
 */

#include <Resources/RHISamplerDesc.hpp>
#include <Tests/UnitTest.hpp>

namespace Lumen
{

LUMEN_TEST( RHISamplerDesc, ValidatesLodAndAnisotropy )
{
    LUMEN_CHECK_TRUE( FRHISamplerDesc{}.Validate().HasValue() );
    FRHISamplerDesc Desc;
    Desc.MinLod = 4.0F;
    Desc.MaxLod = 2.0F;
    LUMEN_CHECK_HAS_ERROR( Desc.Validate() );
    FRHISamplerDesc InvalidAnisotropy;
    InvalidAnisotropy.MaxAnisotropy = 0U;
    LUMEN_CHECK_HAS_ERROR( InvalidAnisotropy.Validate() );
}

} // namespace Lumen
