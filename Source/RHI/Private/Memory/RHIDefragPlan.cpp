/**
 * @file RHIDefragPlan.cpp
 * @brief Implements defragmentation plan validation.
 */

#include <RHI/Memory/RHIDefragPlan.hpp>
#include <RHI/Utilities/RHIAlignment.hpp>

Lumen::TExpected<void, Lumen::RHI::FRHIError> Lumen::RHI::FRHIDefragPlan::Validate () const noexcept
{
    for ( USize First = 0U; First < Moves.size(); ++First )
    {
        if ( not Moves[First].Source.IsValid() or not Moves[First].Destination.IsValid() or
             Moves[First].Source.Size != Moves[First].Destination.Size )
        {
            return TUnexpected<FRHIError>( MakeRHIInvalidMemory( "Invalid defragmentation move" ) );
        }
        for ( USize Second = First + 1U; Second < Moves.size(); ++Second )
        {
            const FRHIAllocation &Left  = Moves[First].Destination;
            const FRHIAllocation &Right = Moves[Second].Destination;
            if ( RHIRangesOverlap( Left.Offset, Left.Size, Right.Offset, Right.Size ) )
            {
                return TUnexpected<FRHIError>( MakeRHIInvalidMemory( "Defragmentation destinations overlap" ) );
            }
        }
    }
    return {};
}
