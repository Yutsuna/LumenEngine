/**
 * @file NullSynchronizationTests.cpp
 * @brief Tests Null binary reuse and debug identity.
 */

#include "Synchronization/NullSynchronization.hpp"
#include <Tests/UnitTest.hpp>

namespace Lumen
{
namespace
{

    LUMEN_TEST( NullSynchronization, BinaryWaitConsumesSignalForReuse )
    {
        Tests::FNullSemaphore Semaphore( { .Type = ERHISemaphoreType::Binary, .InitialValue = 1ULL, .DebugName = "Acquire" },
                                         FRHIObjectId{ 9ULL } );
        LUMEN_CHECK_EQ( Semaphore.GetId().GetValue(), 9ULL );
        LUMEN_CHECK_EQ( Semaphore.GetDebugName(), FString( "Acquire" ) );
        LUMEN_CHECK_TRUE( Semaphore.Wait( 1ULL, 0.0 ).HasValue() );
        LUMEN_CHECK_EQ( Semaphore.GetValue(), 0ULL );
        LUMEN_CHECK_HAS_ERROR( Semaphore.Wait( 1ULL, 0.0 ) );
        LUMEN_CHECK_HAS_ERROR( Semaphore.Signal( 2ULL ) );
        LUMEN_CHECK_TRUE( Semaphore.Signal( 1ULL ).HasValue() );
        LUMEN_CHECK_TRUE( Semaphore.Wait( 1ULL, 0.0 ).HasValue() );
    }

} // namespace
} // namespace Lumen
