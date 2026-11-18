/**
 * @file RHISubmitDescTests.cpp
 * @brief Tests submission semaphore and completion validation.
 */

#include "Synchronization/NullSynchronization.hpp"
#include <Queue/RHISubmitDesc.hpp>
#include <Tests/UnitTest.hpp>
namespace Lumen
{
namespace
{
    LUMEN_TEST( RHISubmitDesc, ValidatesSignalsWaitsDuplicatesAndCompletion )
    {
        TSharedRef<Tests::FNullSemaphore> Semaphore =
            MakeShared<Tests::FNullSemaphore>( FRHISemaphoreDesc{}, FRHIObjectId{ 1ULL } );
        FRHISubmitDesc Desc;
        Desc.Signals    = { { .Semaphore = Semaphore, .Value = 1ULL } };
        Desc.Completion = { 1ULL };
        LUMEN_CHECK_TRUE( Desc.Validate().HasValue() );
        Desc.Waits = { { .Semaphore = Semaphore, .Value = 1ULL } };
        LUMEN_CHECK_TRUE( Desc.Validate().HasValue() );
        Desc.Signals.push_back( { .Semaphore = Semaphore, .Value = 2ULL } );
        LUMEN_CHECK_HAS_ERROR( Desc.Validate() );
        Desc.Signals.pop_back();
        Desc.Completion = {};
        LUMEN_CHECK_HAS_ERROR( Desc.Validate() );
        Desc.Completion       = { 1ULL };
        Desc.Signals[0].Value = 0ULL;
        LUMEN_CHECK_HAS_ERROR( Desc.Validate() );
    }
    LUMEN_TEST( RHISubmitDesc, BinaryOperationsRequireOne )
    {
        TSharedRef<Tests::FNullSemaphore> Semaphore = MakeShared<Tests::FNullSemaphore>(
            FRHISemaphoreDesc{ .Type = ERHISemaphoreType::Binary, .InitialValue = 0ULL, .DebugName = "Binary" },
            FRHIObjectId{ 1ULL } );
        FRHISubmitDesc Desc;
        Desc.Signals    = { { .Semaphore = Semaphore, .Value = 2ULL } };
        Desc.Completion = { 1ULL };
        LUMEN_CHECK_HAS_ERROR( Desc.Validate() );
        Desc.Signals[0].Value = 1ULL;
        LUMEN_CHECK_TRUE( Desc.Validate().HasValue() );
    }

} // namespace
} // namespace Lumen
