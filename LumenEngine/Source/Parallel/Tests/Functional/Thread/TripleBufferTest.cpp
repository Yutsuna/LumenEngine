#include "Thread/TripleBuffer.hpp"
#include "CoreTypes.hpp"

#include <gtest/gtest.h>

#include <atomic>
#include <string>
#include <thread>
#include <vector>

TEST( ParallelTripleBuffer, DefaultConstructorProvidesCleanDefaultSnapshot )
{
    LumenEngine::Parallel::TTripleBuffer<LumenEngine::Int32> Buffer;

    EXPECT_FALSE( Buffer.IsDirty() );
    EXPECT_EQ( Buffer.ReadBuffer(), 0 );
}

TEST( ParallelTripleBuffer, ValueConstructorInitializesAllSlots )
{
    LumenEngine::Parallel::TTripleBuffer<LumenEngine::Int32> Buffer( 42 );

    EXPECT_FALSE( Buffer.IsDirty() );
    EXPECT_EQ( Buffer.ReadBuffer(), 42 );

    Buffer.SwapReadBuffers();
    EXPECT_EQ( Buffer.ReadBuffer(), 42 );
}

TEST( ParallelTripleBuffer, ExternalArrayConstructorStartsDirtyAndCanSwapToTempSlot )
{
    LumenEngine::Int32 External[3] = { 10, 20, 30 };
    LumenEngine::Parallel::TTripleBuffer<LumenEngine::Int32> Buffer( External );

    EXPECT_TRUE( Buffer.IsDirty() );
    EXPECT_EQ( Buffer.ReadBuffer(), 30 );

    Buffer.SwapReadBuffers();

    EXPECT_FALSE( Buffer.IsDirty() );
    EXPECT_EQ( Buffer.ReadBuffer(), 10 );
}

TEST( ParallelTripleBuffer, WriteBufferCopyPublishesAfterReaderSwap )
{
    LumenEngine::Parallel::TTripleBuffer<LumenEngine::Int32> Buffer( 0 );

    Buffer.WriteBuffer( 123 );

    EXPECT_TRUE( Buffer.IsDirty() );
    EXPECT_EQ( Buffer.ReadBuffer(), 0 );

    Buffer.SwapReadBuffers();

    EXPECT_FALSE( Buffer.IsDirty() );
    EXPECT_EQ( Buffer.ReadBuffer(), 123 );
}

TEST( ParallelTripleBuffer, WriteBufferMovePublishesMovedData )
{
    LumenEngine::Parallel::TTripleBuffer<std::string> Buffer( std::string( "initial" ) );

    std::string MovedValue = "moved-value";
    Buffer.WriteBuffer( std::move( MovedValue ) );

    EXPECT_TRUE( Buffer.IsDirty() );

    Buffer.SwapReadBuffers();

    EXPECT_EQ( Buffer.ReadBuffer(), "moved-value" );
}

TEST( ParallelTripleBuffer, SwapWriteBuffersPublishesCurrentWriterSlot )
{
    LumenEngine::Int32 External[3] = { 111, 222, 333 };
    LumenEngine::Parallel::TTripleBuffer<LumenEngine::Int32> Buffer( External );

    Buffer.SwapReadBuffers();
    EXPECT_EQ( Buffer.ReadBuffer(), 111 );

    External[1] = 999;
    Buffer.SwapWriteBuffers();

    EXPECT_TRUE( Buffer.IsDirty() );
    Buffer.SwapReadBuffers();
    EXPECT_EQ( Buffer.ReadBuffer(), 999 );
}

TEST( ParallelTripleBuffer, ConcurrentWritersPublishValidSnapshots )
{
    LumenEngine::Parallel::TTripleBuffer<LumenEngine::Int32> Buffer( 0 );

    constexpr LumenEngine::Int32 WriterCount     = 4;
    constexpr LumenEngine::Int32 WritesPerWriter = 400;
    constexpr LumenEngine::Int32 TotalWrites     = WriterCount * WritesPerWriter;

    std::atomic<LumenEngine::Int32> CompletedWrites = 0;
    std::vector<std::jthread> Writers;
    Writers.reserve( WriterCount );

    for ( LumenEngine::Int32 Writer = 0; Writer < WriterCount; ++Writer )
    {
        Writers.emplace_back(
            [Writer, &Buffer, &CompletedWrites] ()
            {
                for ( LumenEngine::Int32 Index = 0; Index < WritesPerWriter; ++Index )
                {
                    const LumenEngine::Int32 Value = ( Writer * 100000 ) + Index;
                    Buffer.WriteBuffer( Value );
                    CompletedWrites.fetch_add( 1, std::memory_order_relaxed );
                }
            } );
    }

    while ( CompletedWrites.load( std::memory_order_relaxed ) < TotalWrites )
    {
        Buffer.SwapReadBuffers();
        std::this_thread::yield();
    }

    Writers.clear();

    Buffer.SwapReadBuffers();

    const LumenEngine::Int32 Observed = Buffer.ReadBuffer();
    EXPECT_GE( Observed, 0 );
    EXPECT_LT( Observed, ( WriterCount * 100000 ) + WritesPerWriter );
}

TEST( ParallelTripleBuffer, RepeatedSwapReadWhenCleanDoesNotChangeSnapshot )
{
    LumenEngine::Parallel::TTripleBuffer<LumenEngine::Int32> Buffer( 314 );

    for ( int Attempt = 0; Attempt < 20; ++Attempt )
    {
        Buffer.SwapReadBuffers();
        EXPECT_FALSE( Buffer.IsDirty() );
        EXPECT_EQ( Buffer.ReadBuffer(), 314 );
    }
}

TEST( ParallelTripleBuffer, SequentialWritesExposeLatestPublishedValue )
{
    LumenEngine::Parallel::TTripleBuffer<LumenEngine::Int32> Buffer( -1 );

    Buffer.WriteBuffer( 10 );
    Buffer.WriteBuffer( 20 );
    Buffer.WriteBuffer( 30 );

    EXPECT_TRUE( Buffer.IsDirty() );
    Buffer.SwapReadBuffers();

    EXPECT_FALSE( Buffer.IsDirty() );
    EXPECT_EQ( Buffer.ReadBuffer(), 30 );
}

TEST( ParallelTripleBuffer, SwapWriteBuffersWithoutNewWriteStillMarksDirty )
{
    LumenEngine::Parallel::TTripleBuffer<LumenEngine::Int32> Buffer( 404 );

    EXPECT_FALSE( Buffer.IsDirty() );
    Buffer.SwapWriteBuffers();
    EXPECT_TRUE( Buffer.IsDirty() );

    Buffer.SwapReadBuffers();
    EXPECT_FALSE( Buffer.IsDirty() );
    EXPECT_EQ( Buffer.ReadBuffer(), 404 );
}
