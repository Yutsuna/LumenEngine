/**
 * @file RHIDebugSinkTests.cpp
 * @brief Tests diagnostic sink ordering, filtering, and concurrent emission.
 */

#include <Debug/RecordingDebugSink.hpp>
#include <Tests/UnitTest.hpp>
#include <Thread/Thread.hpp>

namespace Lumen
{
namespace
{

    LUMEN_TEST( RHIDebugSink, RecordsOrderedCopiesAndClears )
    {
        Tests::FRecordingDebugSink Sink;
        Sink.Emit( FRHIDebugMessage( ERHIDebugSeverity::Info, ERHIDebugCategory::General, 1U, "first" ) );
        Sink.Emit( FRHIDebugMessage( ERHIDebugSeverity::Error, ERHIDebugCategory::Resource, 2U, "second" ) );
        const auto Messages = Sink.GetMessages();
        LUMEN_CHECK_EQ( Messages.size(), 2U );
        LUMEN_CHECK_EQ( Messages[0].Code, 1U );
        LUMEN_CHECK_EQ( Messages[1].Code, 2U );
        LUMEN_CHECK_TRUE( Sink.ContainsCode( 2U ) );
        LUMEN_CHECK_EQ( Sink.GetCount( ERHIDebugSeverity::Warning ), 1U );
        Sink.Clear();
        LUMEN_CHECK_EQ( Sink.GetCount(), 0U );
    }

    LUMEN_TEST( RHIDebugSink, ConcurrentEmitRetainsEveryMessage )
    {
        Tests::FRecordingDebugSink Sink;
        TVector<FThread> Producers;
        for ( UInt32 Producer = 0U; Producer < 4U; ++Producer )
        {
            Producers.emplace_back( [&Sink, Producer]
            {
                for ( UInt32 Index = 0U; Index < 32U; ++Index )
                {
                    Sink.Emit( FRHIDebugMessage( ERHIDebugSeverity::Warning,
                                                 ERHIDebugCategory::Synchronization,
                                                 Producer * 32U + Index,
                                                 "concurrent" ) );
                }
            } );
        }
        for ( auto &Producer : Producers )
        {
            Producer.Join();
        }
        LUMEN_CHECK_EQ( Sink.GetCount(), 128U );
        LUMEN_CHECK_EQ( Sink.GetCount( ERHIDebugSeverity::Error ), 0U );
        LUMEN_CHECK_TRUE( Sink.ContainsCode( 127U ) );
    }

} // namespace
} // namespace Lumen
