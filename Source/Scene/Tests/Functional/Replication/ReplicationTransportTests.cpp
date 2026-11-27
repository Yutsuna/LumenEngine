/**
 * @file ReplicationTransportTests.cpp
 * @brief In-memory transport contract: FIFO, bounded capacity, deterministic faults, no hot-path allocation.
 */

#include <Replication/ReplicationTransport.hpp>

#include <Tests/UnitTest.hpp>

#include <AllocationTracking.hpp>

#include <Templates/Expected.hpp>
#include <Templates/Vector.hpp>

namespace Lumen
{

namespace
{

    /** @brief Frame size used by the deterministic fault fixtures. */
    constexpr USize TestFrameBytes = 16ULL;

    /**
     * @class FMemoryReplicationTransport
     * @brief Pre-reserved in-memory ring implementing IReplicationTransport for tests only.
     * @details Loss, duplication and reordering are driven by literal seed/period pairs so every failure
     *          pattern is reproducible without randomness or allocation.
     */
    class FMemoryReplicationTransport final : public IReplicationTransport
    {
    public:

        FMemoryReplicationTransport ( const USize InCapacity, const USize InFrameBytes ) noexcept
            : Capacity( InCapacity ), FrameBytes( InFrameBytes )
        {
            Bytes.resize( Capacity * FrameBytes );
            Lengths.resize( Capacity );
        }

        /** @brief Configures periodic faults; a zero period disables that fault. */
        void Configure ( const UInt64 InSeed,
                         const USize InLossPeriod,
                         const USize InDuplicatePeriod,
                         const USize InReorderPeriod ) noexcept
        {
            Seed            = InSeed;
            LossPeriod      = InLossPeriod;
            DuplicatePeriod = InDuplicatePeriod;
            ReorderPeriod   = InReorderPeriod;
        }

        void SetDropAll ( const Bool bInDrop ) noexcept
        {
            bDropAll = bInDrop;
        }

        [[nodiscard]] USize GetPendingCount () const noexcept
        {
            return Count;
        }

        [[nodiscard]] USize GetDroppedSendCount () const noexcept
        {
            return DroppedSendCount;
        }

        [[nodiscard]] TExpected<void, EReplicationError> TrySend ( const TSpan<const Byte> InBytes ) noexcept override
        {
            if ( InBytes.size() > FrameBytes )
            {
                return TUnexpected<EReplicationError>{ EReplicationError::CapacityExceeded };
            }
            ++SentCount;
            if ( bDropAll or MatchesFault( LossPeriod ) )
            {
                ++DroppedSendCount;
                return {};
            }
            if ( Count == Capacity )
            {
                return TUnexpected<EReplicationError>{ EReplicationError::CapacityExceeded };
            }
            Enqueue( InBytes );
            if ( MatchesFault( DuplicatePeriod ) and Count < Capacity )
            {
                Enqueue( InBytes );
            }
            if ( MatchesFault( ReorderPeriod ) and Count >= 2ULL )
            {
                SwapTailTwo();
            }
            return {};
        }

        [[nodiscard]] TExpected<USize, EReplicationError> TryReceive ( const TSpan<Byte> OutBytes ) noexcept override
        {
            if ( Count == 0ULL )
            {
                return 0ULL;
            }
            const USize Length   = Lengths[Head];
            const USize Copied   = Length <= OutBytes.size() ? Length : OutBytes.size();
            const USize SlotBase = Head * FrameBytes;
            for ( USize Index = 0ULL; Index < Copied; ++Index )
            {
                OutBytes[Index] = Bytes[SlotBase + Index];
            }
            Head = ( Head + 1ULL ) % Capacity;
            --Count;
            return Copied;
        }

    private:

        [[nodiscard]] Bool MatchesFault ( const USize InPeriod ) const noexcept
        {
            return InPeriod != 0ULL and ( ( SentCount + Seed ) % static_cast<UInt64>( InPeriod ) ) == 0ULL;
        }

        void Enqueue ( const TSpan<const Byte> InBytes ) noexcept
        {
            const USize Slot     = ( Head + Count ) % Capacity;
            const USize SlotBase = Slot * FrameBytes;
            for ( USize Index = 0ULL; Index < InBytes.size(); ++Index )
            {
                Bytes[SlotBase + Index] = InBytes[Index];
            }
            Lengths[Slot] = InBytes.size();
            ++Count;
        }

        void SwapTailTwo () noexcept
        {
            const USize First      = ( Head + Count - 2ULL ) % Capacity;
            const USize Second     = ( Head + Count - 1ULL ) % Capacity;
            const USize FirstBase  = First * FrameBytes;
            const USize SecondBase = Second * FrameBytes;

            const USize Length = Lengths[First];
            Lengths[First]     = Lengths[Second];
            Lengths[Second]    = Length;
            for ( USize Index = 0ULL; Index < FrameBytes; ++Index )
            {
                const Byte Value          = Bytes[FirstBase + Index];
                Bytes[FirstBase + Index]  = Bytes[SecondBase + Index];
                Bytes[SecondBase + Index] = Value;
            }
        }

        USize Capacity         = 0ULL;
        USize FrameBytes       = 0ULL;
        USize Head             = 0ULL;
        USize Count            = 0ULL;
        USize SentCount        = 0ULL;
        USize DroppedSendCount = 0ULL;
        UInt64 Seed            = 1ULL;
        USize LossPeriod       = 0ULL;
        USize DuplicatePeriod  = 0ULL;
        USize ReorderPeriod    = 0ULL;
        Bool bDropAll          = false;
        TVector<Byte> Bytes;
        TVector<USize> Lengths;
    };

    /** @brief Sends one short frame whose first byte identifies it. */
    void SendMarker ( IReplicationTransport &InTransport, const Byte InMarker ) noexcept
    {
        Byte Frame[TestFrameBytes] = {};
        Frame[0U]                  = InMarker;
        Frame[1U]                  = 0xA5U;
        static_cast<void>( InTransport.TrySend( Frame ) );
    }

    /** @brief Receives one frame and returns its identifying first byte, or 0 when empty. */
    [[nodiscard]] Byte ReceiveMarker ( IReplicationTransport &InTransport, TSpan<Byte> InStorage ) noexcept
    {
        const TExpected<USize, EReplicationError> Received = InTransport.TryReceive( InStorage );
        if ( not Received or *Received == 0ULL )
        {
            return static_cast<Byte>( 0U );
        }
        return InStorage[0U];
    }

} // namespace

LUMEN_TEST( FReplicationTransport, FifoOrderAndEmptyReceive )
{
    FMemoryReplicationTransport Ring( 4ULL, TestFrameBytes );
    IReplicationTransport &Transport = Ring;
    TVector<Byte> Storage;
    Storage.resize( TestFrameBytes );

    LUMEN_CHECK_EQ( Ring.GetPendingCount(), 0ULL );
    LUMEN_CHECK_EQ( ReceiveMarker( Transport, Storage ), 0U );

    for ( Byte Marker = 1U; Marker <= 4U; ++Marker )
    {
        SendMarker( Transport, Marker );
    }
    LUMEN_CHECK_EQ( Ring.GetPendingCount(), 4ULL );

    for ( Byte Marker = 1U; Marker <= 4U; ++Marker )
    {
        const TExpected<USize, EReplicationError> Received = Transport.TryReceive( Storage );
        LUMEN_CHECK_HAS_VALUE( Received );
        LUMEN_CHECK_EQ( *Received, TestFrameBytes );
        LUMEN_CHECK_EQ( static_cast<UInt32>( Storage[0U] ), static_cast<UInt32>( Marker ) );
        LUMEN_CHECK_EQ( static_cast<UInt32>( Storage[1U] ), 0xA5U );
    }
    LUMEN_CHECK_EQ( Ring.GetPendingCount(), 0ULL );
    LUMEN_CHECK_EQ( ReceiveMarker( Transport, Storage ), 0U );
}

LUMEN_TEST( FReplicationTransport, CapacityAcceptsExactlyNAndRejectsNPlusOneWithoutPartialWrite )
{
    FMemoryReplicationTransport Ring( 3ULL, TestFrameBytes );

    for ( Byte Marker = 1U; Marker <= 3U; ++Marker )
    {
        LUMEN_CHECK_HAS_VALUE( Ring.TrySend( TSpan<const Byte>( &Marker, 1ULL ) ) );
    }

    const Byte OverflowMarker                         = 99U;
    const TExpected<void, EReplicationError> Rejected = Ring.TrySend( TSpan<const Byte>( &OverflowMarker, 1ULL ) );
    LUMEN_CHECK_HAS_ERROR( Rejected );
    LUMEN_CHECK_EQ( Rejected.Error(), EReplicationError::CapacityExceeded );
    LUMEN_CHECK_EQ( Ring.GetPendingCount(), 3ULL );
    LUMEN_CHECK_EQ( Ring.GetDroppedSendCount(), 0ULL );

    TVector<Byte> Storage;
    Storage.resize( TestFrameBytes );
    for ( Byte Marker = 1U; Marker <= 3U; ++Marker )
    {
        LUMEN_CHECK_EQ( static_cast<UInt32>( ReceiveMarker( Ring, Storage ) ), static_cast<UInt32>( Marker ) );
    }

    SendMarker( Ring, 4U );
    LUMEN_CHECK_EQ( static_cast<UInt32>( ReceiveMarker( Ring, Storage ) ), 4U );
    LUMEN_CHECK_EQ( ReceiveMarker( Ring, Storage ), 0U );
}

LUMEN_TEST( FReplicationTransport, LossDropDuplicateAndReorderAreSeedDriven )
{
    TVector<Byte> Storage;
    Storage.resize( TestFrameBytes );

    FMemoryReplicationTransport Lossy( 16ULL, TestFrameBytes );
    Lossy.Configure( 7ULL, 3ULL, 0ULL, 0ULL );
    for ( Byte Marker = 1U; Marker <= 6U; ++Marker )
    {
        SendMarker( Lossy, Marker );
    }
    LUMEN_CHECK_EQ( Lossy.GetDroppedSendCount(), 2ULL );
    UInt32 LossReceived = 0U;
    while ( ReceiveMarker( Lossy, Storage ) != static_cast<Byte>( 0U ) )
    {
        ++LossReceived;
    }
    LUMEN_CHECK_EQ( LossReceived, 4ULL );

    FMemoryReplicationTransport Duplicating( 16ULL, TestFrameBytes );
    Duplicating.Configure( 11ULL, 0ULL, 3ULL, 0ULL );
    TVector<Byte> DuplicatedMarkers;
    for ( Byte Marker = 1U; Marker <= 6U; ++Marker )
    {
        SendMarker( Duplicating, Marker );
    }
    while ( const Byte Marker = ReceiveMarker( Duplicating, Storage ) )
    {
        DuplicatedMarkers.push_back( Marker );
    }
    LUMEN_CHECK_EQ( DuplicatedMarkers.size(), 8ULL );
    LUMEN_CHECK_EQ( static_cast<UInt32>( DuplicatedMarkers[0U] ), 1U );
    LUMEN_CHECK_EQ( static_cast<UInt32>( DuplicatedMarkers[1U] ), 1U );

    FMemoryReplicationTransport Reordering( 16ULL, TestFrameBytes );
    Reordering.Configure( 13ULL, 0ULL, 0ULL, 3ULL );
    TVector<Byte> ReorderedMarkers;
    for ( Byte Marker = 1U; Marker <= 6U; ++Marker )
    {
        SendMarker( Reordering, Marker );
    }
    while ( const Byte Marker = ReceiveMarker( Reordering, Storage ) )
    {
        ReorderedMarkers.push_back( Marker );
    }
    LUMEN_CHECK_EQ( ReorderedMarkers.size(), 6ULL );
    LUMEN_CHECK_EQ( static_cast<UInt32>( ReorderedMarkers[0U] ), 2U );
    LUMEN_CHECK_EQ( static_cast<UInt32>( ReorderedMarkers[1U] ), 1U );
    LUMEN_CHECK_EQ( static_cast<UInt32>( ReorderedMarkers[5U] ), 6U );

    FMemoryReplicationTransport Dropping( 4ULL, TestFrameBytes );
    Dropping.SetDropAll( true );
    SendMarker( Dropping, 5U );
    LUMEN_CHECK_EQ( Dropping.GetPendingCount(), 0ULL );
    LUMEN_CHECK_EQ( Dropping.GetDroppedSendCount(), 1ULL );
}

LUMEN_TEST( FReplicationTransport, SendAndReceiveAfterReserveDoNotAllocate )
{
    FMemoryReplicationTransport Ring( 8ULL, 256ULL );
    TVector<Byte> Storage;
    Storage.resize( 256ULL );
    Byte Frame[256] = {};

    const FAllocationTrackingScope Tracking;
    for ( USize Index = 0ULL; Index < 1000ULL; ++Index )
    {
        Frame[0U] = static_cast<Byte>( Index & 0xFFULL );
        LUMEN_CHECK_HAS_VALUE( Ring.TrySend( Frame ) );
        LUMEN_CHECK_HAS_VALUE( Ring.TryReceive( Storage ) );
    }
    LUMEN_CHECK_EQ( Tracking.GetCount(), 0ULL );
}

} // namespace Lumen
