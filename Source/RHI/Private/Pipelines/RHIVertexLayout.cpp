/**
 * @file RHIVertexLayout.cpp
 * @brief Implements vertex layout validation and hashing.
 */

#include <RHI/Pipelines/RHIVertexLayout.hpp>
Lumen::TExpected<void, Lumen::RHI::FRHIError> Lumen::RHI::FRHIVertexLayout::Validate () const noexcept
{
    for ( USize First = 0U; First < Attributes.size(); ++First )
    {
        if ( Attributes[First].Format == ERHIFormat::Unknown or Attributes[First].Format >= ERHIFormat::Count )
        {
            return TUnexpected<FRHIError>( MakeRHIInvalidPipeline( "Unknown vertex format" ) );
        }
        Bool bHasBuffer = false;
        for ( const auto &Buffer : Buffers )
        {
            bHasBuffer = bHasBuffer or Buffer.Slot == Attributes[First].BufferSlot;
        }
        if ( not bHasBuffer )
        {
            return TUnexpected<FRHIError>( MakeRHIInvalidPipeline( "Vertex attribute has no buffer" ) );
        }
        for ( USize Second = First + 1U; Second < Attributes.size(); ++Second )
        {
            if ( Attributes[First].Location == Attributes[Second].Location )
            {
                return TUnexpected<FRHIError>( MakeRHIInvalidPipeline( "Duplicate vertex location" ) );
            }
        }
    }
    for ( USize First = 0U; First < Buffers.size(); ++First )
    {
        if ( Buffers[First].Stride == 0U )
        {
            return TUnexpected<FRHIError>( MakeRHIInvalidPipeline( "Vertex buffer stride is zero" ) );
        }
        for ( USize Second = First + 1U; Second < Buffers.size(); ++Second )
        {
            if ( Buffers[First].Slot == Buffers[Second].Slot )
            {
                return TUnexpected<FRHIError>( MakeRHIInvalidPipeline( "Duplicate vertex buffer slot" ) );
            }
        }
    }
    return {};
}
Lumen::UInt64 Lumen::RHI::FRHIVertexLayout::GetHash () const noexcept
{
    /** NOTE: Hash declared fields only; struct padding is indeterminate and must not affect identity. */
    UInt64 Hash = 0ULL;
    for ( const auto &Buffer : Buffers )
    {
        Hash = RHIHashCombine( Hash, RHIHashValue( Buffer.Slot ) );
        Hash = RHIHashCombine( Hash, RHIHashValue( Buffer.Stride ) );
        Hash = RHIHashCombine( Hash, RHIHashValue( Buffer.Rate ) );
    }
    for ( const auto &Attribute : Attributes )
    {
        Hash = RHIHashCombine( Hash, RHIHashValue( Attribute.Location ) );
        Hash = RHIHashCombine( Hash, RHIHashValue( Attribute.BufferSlot ) );
        Hash = RHIHashCombine( Hash, RHIHashValue( Attribute.Format ) );
        Hash = RHIHashCombine( Hash, RHIHashValue( Attribute.Offset ) );
    }
    return Hash;
}
