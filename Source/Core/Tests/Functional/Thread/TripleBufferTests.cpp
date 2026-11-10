/**
 * @file TripleBufferTests.cpp
 * @brief Functional tests for Lumen::Thread::TTripleBuffer.
 */

#include <Tests/UnitTest.hpp>
#include <Thread/TripleBuffer.hpp>

#include <Core/CoreTypes.hpp>

#include <algorithm>
#include <array>
#include <atomic>
#include <thread>
#include <vector>

namespace Lumen
{

/**
 * API contracts (compile-time)
 */

static_assert( not std::is_copy_constructible_v<Thread::TTripleBuffer<Int32>> and
                   not std::is_copy_assignable_v<Thread::TTripleBuffer<Int32>>,
               "TTripleBuffer must not be copyable" );
static_assert( not std::is_move_constructible_v<Thread::TTripleBuffer<Int32>> and
                   not std::is_move_assignable_v<Thread::TTripleBuffer<Int32>>,
               "TTripleBuffer must not be movable" );
static_assert( std::is_nothrow_destructible_v<Thread::TTripleBuffer<Int32>>, "TTripleBuffer must be noexcept destructible" );
static_assert( noexcept( std::declval<const Thread::TTripleBuffer<Int32> &>().ReadBuffer() ), "ReadBuffer must be noexcept" );
static_assert( noexcept( std::declval<Thread::TTripleBuffer<Int32> &>().SwapReadBuffers() ), "SwapReadBuffers must be noexcept" );
static_assert( noexcept( std::declval<Thread::TTripleBuffer<Int32> &>().SwapWriteBuffers() ),
               "SwapWriteBuffers must be noexcept" );

namespace
{

    /**
     * @struct FCountedPayload
     * @brief Default-constructible payload tracking copies and moves for lifetime contract tests.
     */
    struct FCountedPayload
    {
        inline static Int32 Copies = 0;
        inline static Int32 Moves  = 0;

        Int32 Value = 0;

        FCountedPayload () noexcept = default;

        explicit FCountedPayload ( const Int32 InValue ) noexcept : Value( InValue )
        {
        }

        FCountedPayload ( const FCountedPayload &InOther ) noexcept : Value( InOther.Value )
        {
            ++Copies;
        }

        FCountedPayload ( FCountedPayload &&InOther ) noexcept : Value( InOther.Value )
        {
            InOther.Value = 0;
            ++Moves;
        }

        FCountedPayload &operator=( const FCountedPayload &InOther ) noexcept
        {
            Value = InOther.Value;
            ++Copies;
            return *this;
        }

        FCountedPayload &operator=( FCountedPayload &&InOther ) noexcept
        {
            Value         = InOther.Value;
            InOther.Value = 0;
            ++Moves;
            return *this;
        }
    };

    /** @brief Drains every pending publication on @p InBuffer and records the reader values into @p OutValues. */
    template <typename BufferType>
    void DrainBuffer ( Thread::TTripleBuffer<BufferType> &InBuffer, std::vector<BufferType> &OutValues )
    {
        while ( InBuffer.IsDirty() )
        {
            InBuffer.SwapReadBuffers();
            OutValues.push_back( InBuffer.ReadBuffer() );
        }
    }

} // namespace

/**
 * Construction
 */

LUMEN_TEST( TTripleBuffer_Construction, DefaultInitializesAllSlotsAndStartsClean )
{
    Thread::TTripleBuffer<Int32> Buffer;

    const Int32 *Slots = Buffer.GetBuffers();

    LUMEN_CHECK_TRUE( Slots != nullptr );
    LUMEN_CHECK_EQ( Slots[0], 0 );
    LUMEN_CHECK_EQ( Slots[1], 0 );
    LUMEN_CHECK_EQ( Slots[2], 0 );

    LUMEN_CHECK_EQ( Buffer.ReadBuffer(), 0 );
    LUMEN_CHECK_FALSE( Buffer.IsDirty() );

    /**
     * INFO: Documented initial slot mapping - index 2 is the reader slot
     */
    LUMEN_CHECK_TRUE( &Buffer.ReadBuffer() == &Slots[2] );
}

LUMEN_TEST( TTripleBuffer_Construction, ValueInitializesAllSlots )
{
    Thread::TTripleBuffer<Int32> Buffer( 42 );

    const Int32 *Slots = Buffer.GetBuffers();

    LUMEN_CHECK_EQ( Slots[0], 42 );
    LUMEN_CHECK_EQ( Slots[1], 42 );
    LUMEN_CHECK_EQ( Slots[2], 42 );

    LUMEN_CHECK_EQ( Buffer.ReadBuffer(), 42 );
    LUMEN_CHECK_FALSE( Buffer.IsDirty() );
}

LUMEN_TEST( TTripleBuffer_Construction, WrapsCallerArrayWithoutCopy )
{
    Int32 Slots[3] = { 10, 20, 30 }; /** 0: Temp, 1: Writer, 2: Reader */

    Thread::TTripleBuffer<Int32> Buffer( Slots );

    /** INFO: GetBuffers must be the caller array itself, not a copy */
    LUMEN_CHECK_TRUE( Buffer.GetBuffers() == Slots );
    LUMEN_CHECK_EQ( Buffer.ReadBuffer(), 30 );
    LUMEN_CHECK_TRUE( &Buffer.ReadBuffer() == &Slots[2] );

    /**
     * INFO: Documented mapping - the reader slot starts at index 2, the temp
     * slot at index 0. Swapping for reading promotes the temp slot.
     */
    Buffer.SwapReadBuffers();

    LUMEN_CHECK_EQ( Buffer.ReadBuffer(), 10 );
    LUMEN_CHECK_TRUE( &Buffer.ReadBuffer() == &Slots[0] );

    /** INFO: Writes must land inside caller memory */
    Buffer.WriteBuffer( 77 );

    const Bool bWrittenInsideCallerArray = Slots[0] == 77 or Slots[1] == 77 or Slots[2] == 77;

    LUMEN_CHECK_TRUE( bWrittenInsideCallerArray );

    Buffer.SwapReadBuffers();

    LUMEN_CHECK_EQ( Buffer.ReadBuffer(), 77 );

    const Int32 *ReaderSlot = &Buffer.ReadBuffer();

    LUMEN_CHECK_TRUE( ReaderSlot == &Slots[0] or ReaderSlot == &Slots[1] or ReaderSlot == &Slots[2] );
}

/**
 * Dirty flag
 */

LUMEN_TEST( TTripleBuffer_IsDirty, StartsCleanAfterValueConstruction )
{
    Thread::TTripleBuffer<Int32> Buffer( 1 );

    LUMEN_CHECK_FALSE( Buffer.IsDirty() );
}

LUMEN_TEST( TTripleBuffer_IsDirty, WriteMarksDirty )
{
    Thread::TTripleBuffer<Int32> Buffer( 1 );

    Buffer.WriteBuffer( 2 );

    LUMEN_CHECK_TRUE( Buffer.IsDirty() );
}

LUMEN_TEST( TTripleBuffer_IsDirty, SwapReadClearsDirty )
{
    Thread::TTripleBuffer<Int32> Buffer( 1 );

    Buffer.WriteBuffer( 2 );
    Buffer.SwapReadBuffers();

    LUMEN_CHECK_FALSE( Buffer.IsDirty() );
}

LUMEN_TEST( TTripleBuffer_IsDirty, SwapReadOnCleanBufferIsNoOp )
{
    Thread::TTripleBuffer<Int32> Buffer( 7 );

    Buffer.SwapReadBuffers();

    const Int32 *BeforeSwap   = &Buffer.ReadBuffer();
    const Int32 ExpectedValue = Buffer.ReadBuffer();

    Buffer.SwapReadBuffers();

    LUMEN_CHECK_TRUE( &Buffer.ReadBuffer() == BeforeSwap );
    LUMEN_CHECK_EQ( Buffer.ReadBuffer(), ExpectedValue );
}

/**
 * Read / write
 */

LUMEN_TEST( TTripleBuffer_ReadWrite, WriteIsHiddenUntilSwap )
{
    Thread::TTripleBuffer<Int32> Buffer( 1 );

    Buffer.WriteBuffer( 2 );

    LUMEN_CHECK_EQ( Buffer.ReadBuffer(), 1 );
    LUMEN_CHECK_TRUE( Buffer.IsDirty() );

    Buffer.SwapReadBuffers();

    LUMEN_CHECK_EQ( Buffer.ReadBuffer(), 2 );
    LUMEN_CHECK_FALSE( Buffer.IsDirty() );
}

LUMEN_TEST( TTripleBuffer_ReadWrite, ReaderSeesOnlyLatestOfManyWrites )
{
    Thread::TTripleBuffer<Int32> Buffer( 0 );

    Buffer.WriteBuffer( 1 );
    Buffer.WriteBuffer( 2 );
    Buffer.WriteBuffer( 3 );
    Buffer.WriteBuffer( 4 );
    Buffer.WriteBuffer( 5 );

    Buffer.SwapReadBuffers();

    LUMEN_CHECK_EQ( Buffer.ReadBuffer(), 5 );
    LUMEN_CHECK_FALSE( Buffer.IsDirty() );
}

LUMEN_TEST( TTripleBuffer_ReadWrite, EachSwapExposesNewestPublication )
{
    Thread::TTripleBuffer<Int32> Buffer( 0 );

    Buffer.WriteBuffer( 1 );

    LUMEN_CHECK_EQ( Buffer.ReadBuffer(), 0 );

    Buffer.SwapReadBuffers();

    LUMEN_CHECK_EQ( Buffer.ReadBuffer(), 1 );

    Buffer.WriteBuffer( 2 );

    LUMEN_CHECK_EQ( Buffer.ReadBuffer(), 1 );

    Buffer.SwapReadBuffers();

    LUMEN_CHECK_EQ( Buffer.ReadBuffer(), 2 );
}

LUMEN_TEST( TTripleBuffer_ReadWrite, CopyWriteCopiesOnceAndNeverMoves )
{
    Thread::TTripleBuffer<FCountedPayload> Buffer( FCountedPayload( 1 ) );
    FCountedPayload::Copies = 0;
    FCountedPayload::Moves  = 0;

    const FCountedPayload Source( 5 );

    Buffer.WriteBuffer( Source );

    LUMEN_CHECK_EQ( FCountedPayload::Copies, 1 );
    LUMEN_CHECK_EQ( FCountedPayload::Moves, 0 );

    Buffer.SwapReadBuffers();

    LUMEN_CHECK_EQ( Buffer.ReadBuffer().Value, 5 );
}

LUMEN_TEST( TTripleBuffer_ReadWrite, MoveWriteMovesOnceAndNeverCopies )
{
    Thread::TTripleBuffer<FCountedPayload> Buffer( FCountedPayload( 1 ) );
    FCountedPayload::Copies = 0;
    FCountedPayload::Moves  = 0;

    Buffer.WriteBuffer( FCountedPayload( 5 ) );

    LUMEN_CHECK_EQ( FCountedPayload::Moves, 1 );
    LUMEN_CHECK_EQ( FCountedPayload::Copies, 0 );

    Buffer.SwapReadBuffers();

    LUMEN_CHECK_EQ( Buffer.ReadBuffer().Value, 5 );
}

LUMEN_TEST( TTripleBuffer_ReadWrite, EditorWritesInPlaceWithoutCopyOrMove )
{
    Thread::TTripleBuffer<FCountedPayload> Buffer( FCountedPayload( 0 ) );
    FCountedPayload::Copies = 0;
    FCountedPayload::Moves  = 0;

    Int32 Invocations = 0;

    Buffer.WriteBuffer( [&Invocations] ( FCountedPayload &InSlot )
    {
        ++Invocations;
        InSlot.Value = 42;
    } );

    LUMEN_CHECK_EQ( Invocations, 1 );
    LUMEN_CHECK_EQ( FCountedPayload::Copies, 0 );
    LUMEN_CHECK_EQ( FCountedPayload::Moves, 0 );
    LUMEN_CHECK_TRUE( Buffer.IsDirty() );

    Buffer.SwapReadBuffers();

    LUMEN_CHECK_EQ( Buffer.ReadBuffer().Value, 42 );
}

/**
 * SwapWriteBuffers
 */

LUMEN_TEST( TTripleBuffer_SwapWrite, PublishesWriterSlotWithoutModification )
{
    Thread::TTripleBuffer<Int32> Buffer( 5 );

    Buffer.SwapWriteBuffers();

    LUMEN_CHECK_TRUE( Buffer.IsDirty() );

    Buffer.SwapReadBuffers();

    /** INFO: The writer slot still holds the initial content */
    LUMEN_CHECK_EQ( Buffer.ReadBuffer(), 5 );

    /**
     * After an in-place write of 9, the writer slot is refreshed with stale
     * content again: without any WriteBuffer call, re-publishing must expose
     * the untouched writer slot, not the last published value.
     */
    Buffer.WriteBuffer( [] ( Int32 &InSlot )
    {
        InSlot = 9;
    } );

    Buffer.SwapReadBuffers();

    LUMEN_CHECK_EQ( Buffer.ReadBuffer(), 9 );

    Buffer.SwapWriteBuffers();

    LUMEN_CHECK_TRUE( Buffer.IsDirty() );

    Buffer.SwapReadBuffers();

    LUMEN_CHECK_EQ( Buffer.ReadBuffer(), 5 );
}

LUMEN_TEST( TTripleBuffer_SwapWrite, CountsAsPublicationForTheReader )
{
    Thread::TTripleBuffer<Int32> Buffer( 0 );

    Buffer.SwapWriteBuffers();
    Buffer.SwapReadBuffers();

    LUMEN_CHECK_FALSE( Buffer.IsDirty() );
}

/**
 * GetBuffers
 */

LUMEN_TEST( TTripleBuffer_GetBuffers, PointerIsStableAcrossCalls )
{
    Thread::TTripleBuffer<Int32> Buffer( 3 );

    const Int32 *First  = Buffer.GetBuffers();
    const Int32 *Second = Buffer.GetBuffers();

    LUMEN_CHECK_TRUE( First == Second );

    Buffer.WriteBuffer( 4 );

    LUMEN_CHECK_TRUE( Buffer.GetBuffers() == First );
}

/**
 * Threading - serialized writers, single reader
 */

LUMEN_TEST( TTripleBuffer_Threading, SerializedWritersPublishStrictlyAscendingValues )
{
    constexpr Int32 WriterCount     = 4;
    constexpr Int32 WritesPerThread = 500;
    constexpr Int32 TotalWrites     = WriterCount * WritesPerThread;

    Thread::TTripleBuffer<Int32> Buffer( 0 );

    Int32 SerializedCounter = 0;
    std::vector<Int32> RecordedValues;
    std::atomic<Bool> bAllWritesDone{ false };

    auto Writer = [&] ()
    {
        for ( Int32 WriteIndex = 0; WriteIndex < WritesPerThread; ++WriteIndex )
        {
            Buffer.WriteBuffer( [&SerializedCounter] ( Int32 &InSlot )
            {
                InSlot = ++SerializedCounter;
            } );
        }
    };

    auto Recorder = [&] ()
    {
        for ( ;; )
        {
            if ( Buffer.IsDirty() )
            {
                Buffer.SwapReadBuffers();
                RecordedValues.push_back( Buffer.ReadBuffer() );

                continue;
            }

            if ( bAllWritesDone.load( std::memory_order_acquire ) )
            {
                break;
            }

            std::this_thread::yield();
        }
    };

    std::thread RecorderThread( Recorder );

    std::array<std::thread, WriterCount> WriterThreads = { std::thread( Writer ), std::thread( Writer ), std::thread( Writer ),
                                                           std::thread( Writer ) };

    for ( std::thread &WriterThread : WriterThreads )
    {
        WriterThread.join();
    }

    bAllWritesDone.store( true, std::memory_order_release );
    RecorderThread.join();

    /** INFO: the counter increments happen under the write mutex, values 1..TotalWrites */
    LUMEN_CHECK_EQ( SerializedCounter, TotalWrites );

    DrainBuffer( Buffer, RecordedValues );

    LUMEN_CHECK_EQ( Buffer.ReadBuffer(), TotalWrites );

    Int32 LastRecorded = 0;

    for ( const Int32 Recorded : RecordedValues )
    {
        if ( Recorded <= LastRecorded or Recorded > TotalWrites ) [[unlikely]]
        {
            LUMEN_CHECK_MSG( false, "the serialized writers must publish strictly ascending values 1..N" );

            LastRecorded = 0;
            break;
        }

        LastRecorded = Recorded;
    }
}

LUMEN_TEST( TTripleBuffer_Threading, SingleWriterReaderSeesOnlyNewerMonotonicValues )
{
    constexpr Int32 TotalWrites = 2000;

    Thread::TTripleBuffer<Int32> Buffer( 0 );

    std::vector<Int32> RecordedValues;
    std::atomic<Bool> bAllWritesDone{ false };

    auto Writer = [&] ()
    {
        for ( Int32 Value = 1; Value <= TotalWrites; ++Value )
        {
            Buffer.WriteBuffer( Value );
        }
    };

    auto Recorder = [&] ()
    {
        for ( ;; )
        {
            if ( Buffer.IsDirty() )
            {
                Buffer.SwapReadBuffers();
                RecordedValues.push_back( Buffer.ReadBuffer() );

                continue;
            }

            if ( bAllWritesDone.load( std::memory_order_acquire ) )
            {
                break;
            }

            std::this_thread::yield();
        }
    };

    std::thread RecorderThread( Recorder );
    std::thread WriterThread( Writer );

    WriterThread.join();

    bAllWritesDone.store( true, std::memory_order_release );
    RecorderThread.join();

    DrainBuffer( Buffer, RecordedValues );

    LUMEN_CHECK_EQ( Buffer.ReadBuffer(), TotalWrites );

    Int32 LastRecorded = 0;

    for ( const Int32 Recorded : RecordedValues )
    {
        if ( Recorded < LastRecorded or Recorded > TotalWrites or Recorded == 0 ) [[unlikely]]
        {
            LUMEN_CHECK_MSG( false, "the reader must only ever observe published, non-decreasing values" );

            LastRecorded = 0;
            break;
        }

        LastRecorded = Recorded;
    }
}

LUMEN_TEST( TTripleBuffer_Threading, ConcurrentWritersStayWithinPublishedRange )
{
    constexpr Int32 WriterCount     = 4;
    constexpr Int32 WritesPerThread = 250;
    constexpr Int32 TotalWrites     = WriterCount * WritesPerThread;
    constexpr Int32 ReaderCount     = 2;

    Thread::TTripleBuffer<Int32> Buffer( 0 );

    std::atomic<Int32> PublishedCounter{ 0 };
    std::atomic<Bool> bAllWritesDone{ false };

    std::array<Int32, ReaderCount> MaxSamples     = {};
    std::array<Int32, ReaderCount> ObservedCounts = {};

    auto Writer = [&] ()
    {
        for ( Int32 WriteIndex = 0; WriteIndex < WritesPerThread; ++WriteIndex )
        {
            const Int32 Value = PublishedCounter.fetch_add( 1, std::memory_order_relaxed ) + 1;

            Buffer.WriteBuffer( Value );
        }
    };

    auto Reader = [&] ( const UInt32 ReaderIndex )
    {
        Int32 MaximumSampled = 0;

        for ( ;; )
        {
            const Int32 Sampled = Buffer.ReadBuffer();

            if ( Sampled > MaximumSampled )
            {
                MaximumSampled = Sampled;
            }

            ++ObservedCounts[ReaderIndex];

            if ( bAllWritesDone.load( std::memory_order_acquire ) )
            {
                break;
            }

            std::this_thread::yield();
        }

        MaxSamples[ReaderIndex] = MaximumSampled;
    };

    std::array<std::thread, WriterCount> WriterThreads = { std::thread( Writer ), std::thread( Writer ), std::thread( Writer ),
                                                           std::thread( Writer ) };

    std::array<std::thread, ReaderCount> ReaderThreads = { std::thread( Reader, 0U ), std::thread( Reader, 1U ) };

    for ( std::thread &WriterThread : WriterThreads )
    {
        WriterThread.join();
    }

    bAllWritesDone.store( true, std::memory_order_release );

    for ( std::thread &ReaderThread : ReaderThreads )
    {
        ReaderThread.join();
    }

    LUMEN_CHECK_EQ( PublishedCounter.load( std::memory_order_relaxed ), TotalWrites );

    for ( Int32 ReaderIndex = 0; ReaderIndex < ReaderCount; ++ReaderIndex )
    {
        LUMEN_CHECK_TRUE( ObservedCounts[static_cast<USize>( ReaderIndex )] > 0 );
        LUMEN_CHECK_TRUE( MaxSamples[static_cast<USize>( ReaderIndex )] >= 0 );
        LUMEN_CHECK_TRUE( MaxSamples[static_cast<USize>( ReaderIndex )] <= TotalWrites );
    }

    /** INFO: after a full drain the reader slot holds the last published value */
    std::vector<Int32> DrainValues;
    DrainBuffer<Int32>( Buffer, DrainValues );

    LUMEN_CHECK_EQ( Buffer.ReadBuffer(), TotalWrites );
}

} // namespace Lumen
