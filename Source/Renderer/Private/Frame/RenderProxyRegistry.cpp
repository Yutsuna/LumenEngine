/**
 * @file RenderProxyRegistry.cpp
 * @brief Implements capacity-bounded, deterministic proxy snapshot synchronization.
 */

#include <Frame/RenderProxyRegistry.hpp>

#include <Algo/Any.hpp>
#include <Algo/Sort.hpp>

namespace
{

/** @return Index of the first entry whose id is not less than InId. */
[[nodiscard]] Lumen::USize LowerBound ( const Lumen::TVector<Lumen::FRenderProxyEntry> &InEntries,
                                        const Lumen::FRenderProxyId InId ) noexcept
{
    Lumen::USize First = 0ULL;
    Lumen::USize Count = InEntries.size();
    while ( First < Count )
    {
        const Lumen::USize Middle = First + ( ( Count - First ) / 2ULL );
        if ( InEntries[Middle].ProxyId < InId )
        {
            First = Middle + 1ULL;
        }
        else
        {
            Count = Middle;
        }
    }
    return First;
}

} // namespace

Lumen::FRenderProxyRegistry::FRenderProxyRegistry ( const USize InCapacity ) noexcept : Capacity( InCapacity )
{
    Entries.reserve( InCapacity );
    ScratchEntries.reserve( InCapacity );
}

Lumen::TExpected<void, Lumen::ERenderProxyError>
Lumen::FRenderProxyRegistry::Sync ( const TSpan<const FRenderMeshRecord> InMeshes,
                                    const TSpan<const FRenderTransformRecord> InTransforms,
                                    const UInt64 InSyncSequence,
                                    FRenderProxyStats &OutStats ) noexcept
{
    ScratchEntries.clear();

    FRenderProxyStats Stats;
    for ( const FRenderMeshRecord &Mesh : InMeshes )
    {
        if ( not Mesh.ProxyId.IsValid() or not Mesh.MeshAssetId.IsValid() or Mesh.TransformIndex >= InTransforms.size() )
        {
            ++Stats.Rejected;
            continue;
        }
        if ( InTransforms[Mesh.TransformIndex].ProxyId != Mesh.ProxyId )
        {
            ++Stats.Rejected;
            continue;
        }

        /** INFO: The first valid record for an id wins, so duplicate ids never change the result. */
        const Bool bDuplicate = Algo::AnyOf( ScratchEntries, [&Mesh] ( const FRenderProxyEntry &InEntry ) noexcept -> Bool
        {
            return InEntry.ProxyId == Mesh.ProxyId;
        } );
        if ( bDuplicate )
        {
            continue;
        }
        if ( ScratchEntries.size() >= Capacity )
        {
            return TUnexpected<ERenderProxyError>( ERenderProxyError::CapacityExceeded );
        }

        ScratchEntries.push_back( FRenderProxyEntry{ .ProxyId          = Mesh.ProxyId,
                                                     .MeshAssetId      = Mesh.MeshAssetId,
                                                     .MaterialAssetId  = Mesh.MaterialAssetId,
                                                     .TransformIndex   = Mesh.TransformIndex,
                                                     .Flags            = Mesh.Flags,
                                                     .LastSyncSequence = InSyncSequence } );
    }

    Algo::Sort( ScratchEntries, [] ( const FRenderProxyEntry &InLeft, const FRenderProxyEntry &InRight ) noexcept -> Bool
    {
        return InLeft.ProxyId < InRight.ProxyId;
    } );

    for ( const FRenderProxyEntry &Entry : ScratchEntries )
    {
        const USize Index = LowerBound( Entries, Entry.ProxyId );
        if ( Index < Entries.size() and Entries[Index].ProxyId == Entry.ProxyId )
        {
            ++Stats.Updated;
        }
        else
        {
            ++Stats.Created;
        }
    }
    Stats.Removed = static_cast<UInt32>( Entries.size() - static_cast<USize>( Stats.Updated ) );

    Entries.resize( ScratchEntries.size() );
    for ( USize Index = 0ULL; Index < ScratchEntries.size(); ++Index )
    {
        Entries[Index] = ScratchEntries[Index];
    }

    OutStats = Stats;
    return {};
}

Lumen::TOptional<const Lumen::FRenderProxyEntry *> Lumen::FRenderProxyRegistry::Find ( const FRenderProxyId InId ) const noexcept
{
    const USize Index = LowerBound( Entries, InId );
    if ( Index >= Entries.size() or Entries[Index].ProxyId != InId )
    {
        return {};
    }
    return TOptional<const FRenderProxyEntry *>( &Entries[Index] );
}

Lumen::TSpan<const Lumen::FRenderProxyEntry> Lumen::FRenderProxyRegistry::GetEntries () const noexcept
{
    return TSpan<const FRenderProxyEntry>( Entries.data(), Entries.size() );
}

Lumen::USize Lumen::FRenderProxyRegistry::GetSize () const noexcept
{
    return Entries.size();
}

Lumen::USize Lumen::FRenderProxyRegistry::GetCapacity () const noexcept
{
    return Capacity;
}

void Lumen::FRenderProxyRegistry::Clear () noexcept
{
    Entries.clear();
}
