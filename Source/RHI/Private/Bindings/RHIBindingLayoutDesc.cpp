/**
 * @file RHIBindingLayoutDesc.cpp
 * @brief Implements binding layout validation and hashing.
 */

#include <Algo/Sort.hpp>
#include <Bindings/RHIBindingLayoutDesc.hpp>
Lumen::TExpected<void, Lumen::FRHIError> Lumen::FRHIBindingLayoutDesc::Validate () const noexcept
{
    for ( USize First = 0U; First < Entries.size(); ++First )
    {
        if ( Entries[First].Count == 0U or Entries[First].Stages == ERHIShaderStage::None )
        {
            return TUnexpected<FRHIError>( MakeRHIInvalidArgument( ERHIErrorCondition::Binding, "Invalid binding entry" ) );
        }
        for ( USize Second = First + 1U; Second < Entries.size(); ++Second )
        {
            if ( Entries[First].Slot == Entries[Second].Slot )
            {
                return TUnexpected<FRHIError>( MakeRHIInvalidArgument( ERHIErrorCondition::Binding, "Duplicate binding slot" ) );
            }
        }
    }
    return {};
}
Lumen::UInt64 Lumen::FRHIBindingLayoutDesc::GetHash () const noexcept
{
    TVector<FRHIBindingLayoutEntry> Sorted = Entries;
    Algo::Sort( Sorted, [] ( const auto &Left, const auto &Right )
    {
        return Left.Slot < Right.Slot;
    } );
    UInt64 Hash = 0ULL;
    for ( const auto &Entry : Sorted )
    {
        Hash = RHIHashCombine( Hash, RHIHashValue( Entry.Slot ) );
        Hash = RHIHashCombine( Hash, RHIHashValue( Entry.Type ) );
        Hash = RHIHashCombine( Hash, RHIHashValue( Entry.Count ) );
        Hash = RHIHashCombine( Hash, RHIHashValue( Entry.Stages ) );
        Hash = RHIHashCombine( Hash, RHIHashValue( Entry.bBindless ) );
    }
    return Hash;
}
