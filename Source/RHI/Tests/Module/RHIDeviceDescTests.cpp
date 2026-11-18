/**
 * @file RHIDeviceDescTests.cpp
 * @brief Tests device descriptor queue requests.
 */

#include <RHI/Module/RHIDeviceDesc.hpp>
#include <Tests/UnitTest.hpp>

namespace Lumen
{

namespace RHI
{

    LUMEN_TEST( RHIDeviceDesc, RejectsEmptyDuplicateAndZeroQueues )
    {
        LUMEN_CHECK_TRUE( FRHIDeviceDesc{}.Validate().HasValue() );
        FRHIDeviceDesc Desc;
        Desc.Queues.clear();
        LUMEN_CHECK_HAS_ERROR( Desc.Validate() );
        Desc.Queues = { { ERHIQueueType::Graphics, 1U }, { ERHIQueueType::Graphics, 1U } };
        LUMEN_CHECK_HAS_ERROR( Desc.Validate() );
        Desc.Queues = { { ERHIQueueType::Compute, 0U } };
        LUMEN_CHECK_HAS_ERROR( Desc.Validate() );
    }

} // namespace RHI

} // namespace Lumen
