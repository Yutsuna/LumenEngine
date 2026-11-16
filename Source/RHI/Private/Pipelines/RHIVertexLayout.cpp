/**
 * @file RHIVertexLayout.cpp
 * @brief Implements vertex layout validation and hashing.
 */

#include <Pipelines/RHIVertexLayout.hpp>
Lumen::TExpected<void, Lumen::FRHIError> Lumen::FRHIVertexLayout::Validate () const noexcept
{
    for ( USize First = 0U; First < Attributes.size(); ++First )
    {
        if ( Attributes[First].Format == ERHIFormat::Unknown or Attributes[First].Format >= ERHIFormat::Count )
        {
            return TUnexpected<FRHIError>( MakeRHIInvalidArgument( ERHIErrorCondition::Pipeline, "Unknown vertex format" ) );
        }
        Bool bHasBuffer = false;
        for ( const auto &Buffer : Buffers )
        {
            bHasBuffer = bHasBuffer or Buffer.Slot == Attributes[First].BufferSlot;
        }
        if ( not bHasBuffer )
        {
            return TUnexpected<FRHIError>(
                MakeRHIInvalidArgument( ERHIErrorCondition::Pipeline, "Vertex attribute has no buffer" ) );
        }
        for ( USize Second = First + 1U; Second < Attributes.size(); ++Second )
        {
            if ( Attributes[First].Location == Attributes[Second].Location )
            {
                return TUnexpected<FRHIError>(
                    MakeRHIInvalidArgument( ERHIErrorCondition::Pipeline, "Duplicate vertex location" ) );
            }
        }
    }
    for ( USize First = 0U; First < Buffers.size(); ++First )
    {
        if ( Buffers[First].Stride == 0U )
        {
            return TUnexpected<FRHIError>(
                MakeRHIInvalidArgument( ERHIErrorCondition::Pipeline, "Vertex buffer stride is zero" ) );
        }
        for ( USize Second = First + 1U; Second < Buffers.size(); ++Second )
        {
            if ( Buffers[First].Slot == Buffers[Second].Slot )
            {
                return TUnexpected<FRHIError>(
                    MakeRHIInvalidArgument( ERHIErrorCondition::Pipeline, "Duplicate vertex buffer slot" ) );
            }
        }
    }
    return {};
}
Lumen::UInt64 Lumen::FRHIVertexLayout::GetHash () const noexcept
{
    UInt64 Hash = 0ULL;
    for ( const auto &Buffer : Buffers )
    {
        Hash = RHIHashCombine( Hash, RHIHashValue( Buffer ) );
    }
    for ( const auto &Attribute : Attributes )
    {
        Hash = RHIHashCombine( Hash, RHIHashValue( Attribute ) );
    }
    return Hash;
}
