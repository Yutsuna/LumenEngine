/**
 * @file RenderSnapshotStream.cpp
 * @brief Implementation of the latest-only rendering snapshot stream.
 */

#include <Rendering/RenderSnapshotStream.hpp>

Lumen::FRenderSnapshotStream::FRenderSnapshotStream () noexcept : FRenderSnapshotStream( FRenderSnapshotCapacities{} )
{
    /** Empty */
}

Lumen::FRenderSnapshotStream::FRenderSnapshotStream ( const FRenderSnapshotCapacities &InCapacities ) noexcept
    : Slots{}, SnapshotBuffer( Slots ), Capacities( InCapacities )
{
    for ( FRenderSnapshotStorage &Slot : Slots )
    {
        Slot.Reserve( Capacities );
    }

    /** INFO: The array-backed triple buffer starts dirty; all three empty slots are equivalent and fully reserved. */
    SnapshotBuffer.SwapReadBuffers();
}

Lumen::Bool Lumen::FRenderSnapshotStream::SwapReadBuffers () noexcept
{
    if ( not SnapshotBuffer.IsDirty() )
    {
        return false;
    }

    SnapshotBuffer.SwapReadBuffers();

    const UInt64 ConsumedSequence = SnapshotBuffer.ReadBuffer().Header.SnapshotSequence;
    const UInt64 PreviousSequence = LastConsumedSequence.load( std::memory_order_relaxed );

    if ( ConsumedSequence > PreviousSequence )
    {
        const UInt64 SequenceGap = ConsumedSequence - PreviousSequence;
        if ( SequenceGap > 1ULL )
        {
            DroppedSnapshotCount.fetch_add( SequenceGap - 1ULL, std::memory_order_relaxed );
        }

        LastConsumedSequence.store( ConsumedSequence, std::memory_order_release );
    }

    return true;
}

Lumen::FRenderSnapshotView Lumen::FRenderSnapshotStream::GetReadSnapshot () const noexcept
{
    return SnapshotBuffer.ReadBuffer().GetView();
}

Lumen::Bool Lumen::FRenderSnapshotStream::HasPendingSnapshot () const noexcept
{
    return SnapshotBuffer.IsDirty();
}

Lumen::FRenderSnapshotCapacities Lumen::FRenderSnapshotStream::GetCapacities () const noexcept
{
    return Capacities;
}

Lumen::UInt64 Lumen::FRenderSnapshotStream::GetLastPublishedSequence () const noexcept
{
    return LastPublishedSequence.load( std::memory_order_acquire );
}

Lumen::UInt64 Lumen::FRenderSnapshotStream::GetLastConsumedSequence () const noexcept
{
    return LastConsumedSequence.load( std::memory_order_acquire );
}

Lumen::UInt64 Lumen::FRenderSnapshotStream::GetDroppedSnapshotCount () const noexcept
{
    return DroppedSnapshotCount.load( std::memory_order_acquire );
}
