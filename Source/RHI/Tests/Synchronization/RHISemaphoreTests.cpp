/**
 * @file RHISemaphoreTests.cpp
 * @brief Tests semaphore descriptors and monotonic waits.
 */

#include <Synchronization/NullSynchronization.hpp>
#include <Tests/UnitTest.hpp>

namespace Lumen
{

namespace RHI
{
    namespace
    {

        LUMEN_TEST( RHISemaphore, DescriptorRejectsInvalidTypeAndBinaryInitialValue )
        {
            FRHISemaphoreDesc Desc;
            LUMEN_CHECK_TRUE( Desc.Validate().HasValue() );
            Desc.Type = static_cast<ERHISemaphoreType>( 2U );
            LUMEN_CHECK_HAS_ERROR( Desc.Validate() );
            Desc.Type         = ERHISemaphoreType::Binary;
            Desc.InitialValue = 2ULL;
            LUMEN_CHECK_HAS_ERROR( Desc.Validate() );
        }

        LUMEN_TEST( RHISemaphore, TimelineCannotRegressAndWaitCanTimeOut )
        {
            Tests::FNullSemaphore Semaphore( { .InitialValue = 2ULL, .DebugName = "Timeline" }, FRHIObjectId{ 1ULL } );
            LUMEN_CHECK_EQ( Semaphore.GetValue(), 2ULL );
            LUMEN_CHECK_TRUE( Semaphore.Wait( 2ULL, 0.0 ).HasValue() );
            LUMEN_CHECK_HAS_ERROR( Semaphore.Signal( 1ULL ) );
            LUMEN_CHECK_TRUE( Semaphore.Signal( 3ULL ).HasValue() );
            LUMEN_CHECK_TRUE( Semaphore.Wait( 3ULL, 0.0 ).HasValue() );
            FRHIExpected Timeout = Semaphore.Wait( 4ULL, 0.0 );
            LUMEN_CHECK_HAS_ERROR( Timeout );
            LUMEN_CHECK_EQ( Timeout.Error().Code, ERHIErrorCode::Timeout );
        }

    } // namespace
} // namespace RHI

} // namespace Lumen
