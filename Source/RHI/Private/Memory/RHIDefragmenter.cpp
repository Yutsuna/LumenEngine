/**
 * @file RHIDefragmenter.cpp
 * @brief Implements conservative defragmentation planning.
 */
#include "RHIDefragmenter.hpp"
#include <Templates/Hash.hpp>
Lumen::FRHIDefragmenter::FRHIDefragmenter ( const FRHIObjectId InId ) noexcept
    : Identity( InId, FHash::TypeHash<FRHIDefragmenter>() )
{
}
LUMEN_RHI_OBJECT_FORWARDERS( FRHIDefragmenter )
Lumen::TExpected<Lumen::FRHIDefragPlan, Lumen::FRHIError>
Lumen::FRHIDefragmenter::BuildPlan ( const TSpan<const FRHIAllocation> InAllocations ) noexcept
{
    for ( const FRHIAllocation &Allocation : InAllocations )
    {
        if ( not Allocation.IsValid() )
        {
            return TUnexpected<FRHIError>( MakeRHIInvalidMemory( "Invalid live allocation" ) );
        }
    }
    return FRHIDefragPlan{};
}
Lumen::TExpected<void, Lumen::FRHIError> Lumen::FRHIDefragmenter::Execute ( const FRHIDefragPlan &InPlan ) noexcept
{
    return InPlan.Validate();
}
