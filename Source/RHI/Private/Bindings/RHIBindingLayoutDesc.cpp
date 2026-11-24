/**
 * @file RHIBindingLayoutDesc.cpp
 * @brief Implements binding layout validation and hashing.
 */

#include <Algo/ForEach.hpp>
#include <Algo/Sort.hpp>

#include <RHI/Bindings/RHIBindingLayoutDesc.hpp>

/**
 * Public
 */

Lumen::TExpected<void, Lumen::RHI::FRHIError> Lumen::RHI::FRHIBindingLayoutDesc::Validate () const noexcept
{
    for ( USize First = 0U; First < Entries.size(); ++First )
    {
        if ( Entries[First].Count == 0U or Entries[First].Stages == ERHIShaderStage::None )
        {
            return TUnexpected<FRHIError>( MakeRHIInvalidBinding( "Invalid binding entry" ) );
        }
        for ( USize Second = First + 1U; Second < Entries.size(); ++Second )
        {
            if ( Entries[First].Slot == Entries[Second].Slot )
            {
                return TUnexpected<FRHIError>( MakeRHIInvalidBinding( "Duplicate binding slot" ) );
            }
        }
    }
    return {};
}

Lumen::UInt64 Lumen::RHI::FRHIBindingLayoutDesc::GetHash () const noexcept
{
    TVector<FRHIBindingLayoutEntry> Sorted = Entries;
    Algo::Sort( Sorted, [] ( const FRHIBindingLayoutEntry &Left, const FRHIBindingLayoutEntry &Right ) noexcept -> Bool
    {
        return Left.Slot < Right.Slot;
    } );

    UInt64 Hash = 0ULL;

    Algo::ForEach( Sorted, [&Hash] ( const FRHIBindingLayoutEntry &Entry ) noexcept -> void
    {
        Hash = RHIHashCombine( Hash, RHIHashValue( Entry.Slot ) );
        Hash = RHIHashCombine( Hash, RHIHashValue( Entry.Type ) );
        Hash = RHIHashCombine( Hash, RHIHashValue( Entry.Count ) );
        Hash = RHIHashCombine( Hash, RHIHashValue( Entry.Stages ) );
        Hash = RHIHashCombine( Hash, RHIHashValue( Entry.bBindless ) );
    } );
    return Hash;
}
