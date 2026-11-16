/**
 * @file NullBindings.cpp
 * @brief Implements Null binding and descriptor objects.
 */

#include "Bindings/NullBindings.hpp"
#include <Templates/Hash.hpp>

Lumen::Tests::FNullBindingLayout::FNullBindingLayout ( const FRHIBindingLayoutDesc &InDesc, const FRHIObjectId InId ) noexcept
    : Identity( InId, FHash::TypeHash<FNullBindingLayout>() ), Desc( InDesc )
{
    /* */
}

LUMEN_RHI_OBJECT_FORWARDERS( Tests::FNullBindingLayout )
const Lumen::FRHIBindingLayoutDesc &Lumen::Tests::FNullBindingLayout::GetDesc () const noexcept
{
    return Desc;
}

Lumen::Tests::FNullBindingSet::FNullBindingSet ( TSharedRef<IRHIBindingLayout> InLayout, const FRHIObjectId InId ) noexcept
    : Identity( InId, FHash::TypeHash<FNullBindingSet>() ), Layout( std::move( InLayout ) )
{
}

LUMEN_RHI_OBJECT_FORWARDERS( Tests::FNullBindingSet )
Lumen::TSharedRef<Lumen::IRHIBindingLayout> Lumen::Tests::FNullBindingSet::GetLayout () const noexcept
{
    return Layout;
}

Lumen::TExpected<void, Lumen::FRHIError>
Lumen::Tests::FNullBindingSet::Write ( const TSpan<const FRHIBindingWrite> InWrites ) noexcept
{
    for ( const FRHIBindingWrite &Write : InWrites )
    {
        if ( not Write.Validate() )
        {
            return TUnexpected<FRHIError>( MakeRHIInvalidArgument( ERHIErrorCondition::Binding, "Invalid binding write" ) );
        }
        Bool bFound = false;
        for ( const auto &Entry : Layout->GetDesc().Entries )
        {
            bFound = bFound or Entry.Slot == Write.Slot;
        }
        if ( not bFound )
        {
            return TUnexpected<FRHIError>(
                MakeRHIInvalidArgument( ERHIErrorCondition::Binding, "Binding slot is absent from layout" ) );
        }
    }
    Writes.assign( InWrites.begin(), InWrites.end() );
    bDirty = true;
    return {};
}

Lumen::Bool Lumen::Tests::FNullBindingSet::IsDirty () const noexcept
{
    return bDirty;
}

void Lumen::Tests::FNullBindingSet::ClearDirty () noexcept
{
    bDirty = false;
}

Lumen::Tests::FNullDescriptorPoolBackend::FNullDescriptorPoolBackend () noexcept = default;
Lumen::TExpected<Lumen::UInt64, Lumen::FRHIError>
Lumen::Tests::FNullDescriptorPoolBackend::CreatePool ( const UInt32 InCapacity ) noexcept
{
    if ( InCapacity == 0U )
    {
        return TUnexpected<FRHIError>( MakeRHIInvalidArgument( ERHIErrorCondition::Binding, "Null pool capacity is zero" ) );
    }
    return NextPool++;
}

void Lumen::Tests::FNullDescriptorPoolBackend::ReleasePool ( const UInt64 ) noexcept
{
    /* */
}

Lumen::Tests::FNullDescriptorAllocator::FNullDescriptorAllocator ( const FRHIDescriptorAllocatorDesc &InDesc,
                                                                   IRHIDescriptorPoolBackend &InBackend,
                                                                   const FRHIObjectId InId ) noexcept
    : FRHIDescriptorAllocatorBase( InDesc, InBackend ), Identity( InId, FHash::TypeHash<FNullDescriptorAllocator>() )
{
    /* */
}

LUMEN_RHI_OBJECT_FORWARDERS( Tests::FNullDescriptorAllocator )
