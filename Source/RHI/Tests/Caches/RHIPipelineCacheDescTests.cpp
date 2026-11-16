/**
 * @file RHIPipelineCacheDescTests.cpp
 * @brief Tests pipeline cache limits and initial-data validation.
 */

#include <Caches/RHIPipelineCacheDesc.hpp>
#include <Templates/Array.hpp>
#include <Tests/UnitTest.hpp>

namespace Lumen
{
namespace
{

    LUMEN_TEST( RHIPipelineCacheDesc, ValidatesCapacityVersionAndInitialBytes )
    {
        LUMEN_CHECK_TRUE( FRHIPipelineCacheDesc{}.Validate().HasValue() );
        FRHIPipelineCacheDesc Desc;
        Desc.MaxEntries = 0U;
        LUMEN_CHECK_HAS_ERROR( Desc.Validate() );
        Desc.MaxEntries = 1U;
        Desc.MaxBytes   = 0ULL;
        LUMEN_CHECK_HAS_ERROR( Desc.Validate() );
        Desc.MaxBytes = 4ULL;
        const TArray<Byte, 5U> Initial = {};
        Desc.InitialData = Initial;
        LUMEN_CHECK_HAS_ERROR( Desc.Validate() );
        Desc.MaxBytes = 5ULL;
        LUMEN_CHECK_TRUE( Desc.Validate().HasValue() );
        Desc.BackendVersion = 0ULL;
        LUMEN_CHECK_HAS_ERROR( Desc.Validate() );
    }

} // namespace
} // namespace Lumen
