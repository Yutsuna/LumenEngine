/**
 * @file LinearAllocatorTest.cpp
 * @brief Unit tests for FLinearAllocator in LumenEngine
 */

#include "HAL/Memory/LinearAllocator.hpp"
#include "Container/UniquePtr.hpp"
#include "CoreTypes.hpp"
#include "HAL/PlatformTime.hpp"

#include <gtest/gtest.h>
#include <iostream>
#include <thread>
#include <vector>

TEST( HALMemoryLinearAllocator, AllocationAndReset )
{
    using namespace LumenEngine;
    using namespace LumenEngine::HAL;

    alignas( 16 ) Byte Buffer[1024];
    FLinearAllocator Allocator( Buffer, 1024ULL );

    void *Ptr1 = Allocator.Allocate( 128ULL );
    EXPECT_NE( Ptr1, nullptr );
    EXPECT_EQ( Allocator.GetUsedMemory(), 128ULL );

    void *Ptr2 = Allocator.Allocate( 128ULL );
    EXPECT_NE( Ptr2, nullptr );
    EXPECT_EQ( Allocator.GetUsedMemory(), 256ULL );
    EXPECT_EQ( Allocator.GetHighWatermark(), 256ULL );
    EXPECT_EQ( static_cast<Byte *>( Ptr2 ) - static_cast<Byte *>( Ptr1 ), static_cast<ISize>( 128 ) );

    Allocator.Reset();
    EXPECT_EQ( Allocator.GetUsedMemory(), 0ULL );
    EXPECT_EQ( Allocator.GetHighWatermark(), 256ULL );

    void *Ptr3 = Allocator.Allocate( 64ULL );
    EXPECT_EQ( Ptr3, Ptr1 );
    EXPECT_EQ( Allocator.GetHighWatermark(), 256ULL );
}

TEST( HALMemoryLinearAllocator, Alignment )
{
    using namespace LumenEngine;
    using namespace LumenEngine::HAL;

    alignas( 16 ) Byte Buffer[1024];
    FLinearAllocator Allocator( Buffer, 1024ULL );

    // Initial allocation
    ( void )Allocator.Allocate( 1ULL );
    EXPECT_EQ( Allocator.GetUsedMemory(), 1ULL );

    // Next allocation with 16-byte alignment
    void *Ptr = Allocator.Allocate( 16ULL, 16ULL );
    EXPECT_EQ( reinterpret_cast<UPtr>( Ptr ) % 16ULL, reinterpret_cast<UPtr>( Buffer ) % 16ULL );
    EXPECT_EQ( Allocator.GetUsedMemory(), 32ULL );
}

TEST( HALMemoryLinearAllocator, OutOfMemory )
{
    using namespace LumenEngine;
    using namespace LumenEngine::HAL;

    Byte Buffer[64];
    FLinearAllocator Allocator( Buffer, 64ULL );

    void *Ptr1 = Allocator.Allocate( 64ULL );
    EXPECT_NE( Ptr1, nullptr );

    void *Ptr2 = Allocator.Allocate( 1ULL );
    EXPECT_EQ( Ptr2, nullptr );
}

TEST( HALMemoryLinearAllocator, NewObject )
{
    using namespace LumenEngine;
    using namespace LumenEngine::HAL;

    struct FTestObject
    {
        Int32 Value;
        FTestObject ( Int32 InValue ) : Value( InValue )
        {
        }
    };

    alignas( alignof( FTestObject ) ) Byte Buffer[1024];
    FLinearAllocator Allocator( Buffer, 1024ULL );

    FTestObject *Obj = Allocator.New<FTestObject>( 42 );
    ASSERT_NE( Obj, nullptr );
    EXPECT_EQ( Obj->Value, 42 );
}

TEST( HALMemoryLinearAllocator, ScopeGuard )
{
    using namespace LumenEngine;
    using namespace LumenEngine::HAL;

    Byte Buffer[1024];
    FLinearAllocator Allocator( Buffer, 1024ULL );

    // Use alignment of 1 to avoid surprises in math
    ( void )Allocator.Allocate( 100ULL, 1ULL );
    USize OffsetBefore = Allocator.GetUsedMemory();

    {
        FScopeLinear Scope( Allocator );
        ( void )Allocator.Allocate( 200ULL, 1ULL );
        EXPECT_EQ( Allocator.GetUsedMemory(), OffsetBefore + 200ULL );
    }

    EXPECT_EQ( Allocator.GetUsedMemory(), OffsetBefore );
}

TEST( HALMemoryLinearAllocator, NestedScopeGuard )
{
    using namespace LumenEngine;
    using namespace LumenEngine::HAL;

    Byte Buffer[1024];
    FLinearAllocator Allocator( Buffer, 1024ULL );

    ( void )Allocator.Allocate( 100ULL, 1ULL );
    {
        FScopeLinear Scope1( Allocator );
        ( void )Allocator.Allocate( 100ULL, 1ULL );
        EXPECT_EQ( Allocator.GetUsedMemory(), 200ULL );
        {
            FScopeLinear Scope2( Allocator );
            ( void )Allocator.Allocate( 100ULL, 1ULL );
            EXPECT_EQ( Allocator.GetUsedMemory(), 300ULL );
        }
        EXPECT_EQ( Allocator.GetUsedMemory(), 200ULL );
    }
    EXPECT_EQ( Allocator.GetUsedMemory(), 100ULL );
    EXPECT_EQ( Allocator.GetHighWatermark(), 300ULL );
}

TEST( HALMemoryLinearAllocator, OverflowProtection )
{
    using namespace LumenEngine;
    using namespace LumenEngine::HAL;

    Byte Buffer[1024];
    FLinearAllocator Allocator( Buffer, 1024ULL );

    // Try to allocate more than possible with a huge size
    void *Ptr1 = Allocator.Allocate( ~0ULL );
    EXPECT_EQ( Ptr1, nullptr );

    // Note: Overflow in AlignUp is hard to catch without extra checks in AlignUp itself.
    // But we can check if it returns nullptr for impossible alignments.
    // Current implementation doesn't handle huge alignments well if they overflow USize.
}

TEST( HALMemoryLinearAllocator, NullBuffer )
{
    using namespace LumenEngine;
    using namespace LumenEngine::HAL;

    FLinearAllocator Allocator( nullptr, 1024ULL );
    void *Ptr = Allocator.Allocate( 1ULL );
    EXPECT_EQ( Ptr, nullptr );
}

TEST( HALMemoryLinearAllocator, EdgeCases )
{
    using namespace LumenEngine;
    using namespace LumenEngine::HAL;

    Byte Buffer[1024];
    FLinearAllocator Allocator( Buffer, 1024ULL );

    // 1. Zero size allocation
    USize UsedBefore = Allocator.GetUsedMemory();
    void *Ptr0       = Allocator.Allocate( 0ULL );
    EXPECT_NE( Ptr0, nullptr );
    EXPECT_EQ( Allocator.GetUsedMemory(), UsedBefore );

    // 2. Exact fit
    Allocator.Reset();
    void *PtrFull = Allocator.Allocate( 1024ULL, 1ULL );
    EXPECT_NE( PtrFull, nullptr );
    EXPECT_EQ( Allocator.GetUsedMemory(), 1024ULL );
    EXPECT_EQ( Allocator.Allocate( 1ULL ), nullptr );

    // 3. Large alignment
    Byte LargeBuffer[4096];
    FLinearAllocator LargeAllocator( LargeBuffer, 4096ULL );
    ( void )LargeAllocator.Allocate( 1ULL, 1ULL );
    void *PtrPage = LargeAllocator.Allocate( 1ULL, 1024ULL );
    EXPECT_NE( PtrPage, nullptr );
    EXPECT_EQ( LargeAllocator.GetUsedMemory(), 1025ULL );

    // 4. Alignment 1
    void *PtrAlign1 = LargeAllocator.Allocate( 1ULL, 1ULL );
    EXPECT_NE( PtrAlign1, nullptr );
    EXPECT_EQ( LargeAllocator.GetUsedMemory(), 1026ULL );
}

TEST( HALMemoryLinearAllocator, Performance )
{
    using namespace LumenEngine;
    using namespace LumenEngine::HAL;

    const USize NumAllocations = 1000000;
    const USize AllocSize      = 16ULL;
    const USize TotalRequired  = NumAllocations * ( AllocSize + 16ULL );

    TUniquePtr<Byte[]> Buffer = MakeUnique<Byte[]>( TotalRequired );
    FLinearAllocator Allocator( Buffer.Get(), TotalRequired );

    Float64 Start = FPlatformTime::Seconds();
    for ( USize i = 0; i < NumAllocations; ++i )
    {
        void *Ptr = Allocator.Allocate( AllocSize );
        // Touch memory to prevent optimization
        if ( Ptr )
        {
            *static_cast<volatile Byte *>( Ptr ) = static_cast<Byte>( i & 0xFF );
        }
    }
    Float64 End = FPlatformTime::Seconds();

    std::cout << "[          ] LinearAllocator: " << NumAllocations << " allocations took " << ( End - Start ) * 1000.0 << " ms" << std::endl;
}

TEST( HALMemoryLinearAllocator, ParallelAllocators )
{
    using namespace LumenEngine;
    using namespace LumenEngine::HAL;

    const Int32 NumThreads           = 8;
    const USize AllocationsPerThread = 10000;
    std::vector<std::jthread> Threads;
    TAtomic<Int32> SuccessCount = 0;

    for ( Int32 i = 0; i < NumThreads; ++i )
    {
        Threads.emplace_back(
            [&SuccessCount] ()
            {
                // Use heap buffer for thread safety and to avoid stack issues
                auto LocalBuffer = MakeUnique<Byte[]>( 1024 * 256 );
                FLinearAllocator Allocator( LocalBuffer.Get(), 1024 * 256 );

                Bool bThreadSuccess = true;
                for ( USize j = 0; j < AllocationsPerThread; ++j )
                {
                    void *Ptr = Allocator.Allocate( 8ULL );
                    if ( !Ptr )
                    {
                        bThreadSuccess = false;
                        break;
                    }
                    *static_cast<volatile UInt64 *>( Ptr ) = j;
                }

                if ( bThreadSuccess )
                {
                    SuccessCount.fetch_add( 1, std::memory_order_relaxed );
                }
            } );
    }

    Threads.clear(); // jthread joins automatically

    EXPECT_EQ( SuccessCount.load(), NumThreads );
}
