/**
 * @file NullMemory.cpp
 * @brief Implements the simulated Null heap.
 */

#include <Memory/NullMemory.hpp>
#include <Templates/Hash.hpp>

/**
 * Public
 */

Lumen::RHI::Tests::FNullHeap::FNullHeap ( const FRHIHeapDesc &InDesc,
                                          const FRHIObjectId InId,
                                          const FRHIObjectId InOwnerId ) noexcept
    : Identity( InId, FHash::TypeHash<FNullHeap>() ), OwnerId( InOwnerId ), Desc( InDesc ),
      Storage( MakeUnique<Byte[]>( static_cast<USize>( InDesc.Size ) ) )
{
    /* */
}

LUMEN_RHI_OBJECT_FORWARDERS( RHI::Tests, FNullHeap )

Lumen::RHI::FRHIObjectId Lumen::RHI::Tests::FNullHeap::GetOwnerId () const noexcept
{
    return OwnerId;
}

const Lumen::RHI::FRHIHeapDesc &Lumen::RHI::Tests::FNullHeap::GetDesc () const noexcept
{
    return Desc;
}

Lumen::TExpected<Lumen::TSpan<Lumen::Byte>, Lumen::RHI::FRHIError>
Lumen::RHI::Tests::FNullHeap::Map ( const UInt64 InOffset, const UInt64 InSize ) noexcept
{
    if ( not IsRHICpuAccessible( Desc.MemoryClass ) or InOffset > Desc.Size or InSize > Desc.Size - InOffset )
    {
        return TUnexpected<FRHIError>( MakeRHIInvalidMemory( "Null heap mapping is outside or not CPU visible" ) );
    }
    return TSpan<Byte>( Storage.Get() + static_cast<USize>( InOffset ), static_cast<USize>( InSize ) );
}
