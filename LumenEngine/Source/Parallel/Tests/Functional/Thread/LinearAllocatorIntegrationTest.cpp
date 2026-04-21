/**
 * @file LinearAllocatorIntegrationTest.cpp
 * @brief Integration tests for FLinearAllocator with FWorkerPool
 */

#include "CoreTypes.hpp"
#include "HAL/Memory/LinearAllocator.hpp"
#include "Thread/WorkerPool.hpp"

#include <gtest/gtest.h>

TEST( ParallelWorkerPool, LinearAllocatorIntegration )
{
    using namespace LumenEngine;
    using namespace LumenEngine::Parallel;

    FWorkerPool Pool( 4U, 64ULL );
    TAtomic<Bool> bSuccess = true;

    for ( UInt32 Index = 0; Index < 100; ++Index )
    {
        Pool.Submit(
            [&bSuccess] ()
            {
                HAL::FLinearAllocator &Allocator = FWorkerPool::GetWorkerAllocator();

                // At the start of the task, it should be empty because of FScopeLinear in WorkerLoop
                if ( Allocator.GetUsedMemory() != 0ULL )
                {
                    bSuccess = false;
                }

                void *Ptr = Allocator.Allocate( 1024ULL );
                if ( Ptr == nullptr )
                {
                    bSuccess = false;
                    return;
                }

                if ( Allocator.GetUsedMemory() != 1024ULL )
                {
                    bSuccess = false;
                }
            } );
    }

    Pool.WaitAll();
    EXPECT_TRUE( bSuccess.load() );
}
