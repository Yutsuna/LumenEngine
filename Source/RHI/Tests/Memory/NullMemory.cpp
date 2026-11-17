/**
 * @file NullMemory.cpp
 * @brief Implements the simulated Null heap.
 */

#include <Memory/NullMemory.hpp>
#include <Templates/Hash.hpp>

/**
 * Public
 */

Lumen::Tests::FNullHeap::FNullHeap ( const FRHIHeapDesc &InDesc, const FRHIObjectId InId ) noexcept
    : Identity( InId, FHash::TypeHash<FNullHeap>() ), Desc( InDesc ),
      Storage( MakeUnique<Byte[]>( static_cast<USize>( InDesc.Size ) ) )
{
    /* */
}

LUMEN_RHI_OBJECT_FORWARDERS( Tests::FNullHeap )
const Lumen::FRHIHeapDesc &Lumen::Tests::FNullHeap::GetDesc () const noexcept
{
    return Desc;
}

Lumen::TExpected<Lumen::TSpan<Lumen::Byte>, Lumen::FRHIError> Lumen::Tests::FNullHeap::Map ( const UInt64 InOffset,
                                                                                             const UInt64 InSize ) noexcept
{
    if ( not IsRHICpuAccessible( Desc.MemoryClass ) or InOffset > Desc.Size or InSize > Desc.Size - InOffset )
    {
        return TUnexpected<FRHIError>( MakeRHIInvalidMemory( "Null heap mapping is outside or not CPU visible" ) );
    }
    return TSpan<Byte>( Storage.Get() + static_cast<USize>( InOffset ), static_cast<USize>( InSize ) );
}
