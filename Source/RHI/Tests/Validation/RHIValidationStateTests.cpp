/**
 * @file RHIValidationStateTests.cpp
 * @brief Tests shared validation registries and concurrent read access.
 */
#include "Validation/RHIValidationDevice.hpp"
#include "Validation/RHIValidationTestUtils.hpp"
#include <Tests/UnitTest.hpp>
#include <Thread/Thread.hpp>

namespace Lumen
{
namespace
{
    LUMEN_TEST( RHIValidationState, TracksCategoriesAndRemovesDestroyedObjects )
    {
        Tests::FRecordingDebugSink Sink;
        TSharedRef<IRHIDevice> Device = Tests::Validation::MakeDevice( Sink );
        auto *Validation              = dynamic_cast<FRHIValidationDevice *>( Device.Get() );
        LUMEN_CHECK_TRUE( Validation != nullptr );
        const auto &State        = Validation->GetState();
        const USize InitialCount = State->GetLiveObjectCount();
        {
            TSharedRef<IRHIBuffer> Buffer = Tests::Validation::MakeBuffer( Device );
            LUMEN_CHECK_TRUE( State->Contains( *Buffer ) );
            LUMEN_CHECK_TRUE( State->ValidateObject( *Buffer, ERHIValidationObjectKind::Buffer ).HasValue() );
            LUMEN_CHECK_HAS_ERROR( State->ValidateObject( *Buffer, ERHIValidationObjectKind::Texture ) );
            LUMEN_CHECK_EQ( State->GetLiveObjectCount(), InitialCount + 1U );
            LUMEN_CHECK_EQ( *State->GetResourceState( *Buffer ), ERHIResourceState::Common );
        }
        LUMEN_CHECK_EQ( State->GetLiveObjectCount(), InitialCount );
    }

    LUMEN_TEST( RHIValidationState, SupportsConcurrentRegistryReaders )
    {
        Tests::FRecordingDebugSink Sink;
        TSharedRef<IRHIDevice> Device = Tests::Validation::MakeDevice( Sink );
        TSharedRef<IRHIBuffer> Buffer = Tests::Validation::MakeBuffer( Device );
        auto *Validation              = dynamic_cast<FRHIValidationDevice *>( Device.Get() );
        const auto &State             = Validation->GetState();
        TAtomic<UInt32> Failures( 0U );
        TVector<FThread> Readers;
        for ( UInt32 Index = 0U; Index < 4U; ++Index )
        {
            Readers.emplace_back( [&State, &Buffer, &Failures]
            {
                for ( UInt32 Iteration = 0U; Iteration < 1000U; ++Iteration )
                {
                    if ( not State->Contains( *Buffer ) or State->GetLiveObjectCount() < 1U )
                    {
                        Failures.fetch_add( 1U, std::memory_order_relaxed );
                    }
                }
            } );
        }
        for ( auto &Reader : Readers )
        {
            Reader.Join();
        }
        LUMEN_CHECK_EQ( Failures.load(), 0U );
    }
} // namespace
} // namespace Lumen
