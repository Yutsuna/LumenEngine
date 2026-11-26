/**
 * @file RHISubresourceRange.cpp
 * @brief Implements subresource range containment and intersection.
 */

#include <Maths/Macros.hpp>

#include <RHI/Resources/RHISubresourceRange.hpp>
#include <RHI/Utilities/RHICheckedMath.hpp>

/**
 * Public
 */

Lumen::Bool Lumen::RHI::FRHISubresourceRange::IsContainedIn ( const UInt32 InMipCount, const UInt32 InLayerCount ) const noexcept
{
    const TOptional<UInt32> MipEnd   = RHICheckedAdd( BaseMip, MipCount );
    const TOptional<UInt32> LayerEnd = RHICheckedAdd( BaseLayer, LayerCount );

    return IsValid() and MipEnd and LayerEnd and *MipEnd <= InMipCount and *LayerEnd <= InLayerCount;
}

Lumen::TOptional<Lumen::RHI::FRHISubresourceRange>
Lumen::RHI::FRHISubresourceRange::Intersect ( const FRHISubresourceRange &InOther ) const noexcept
{
    const UInt32 MipBegin                 = Maths::Max( BaseMip, InOther.BaseMip );
    const UInt32 LayerBegin               = Maths::Max( BaseLayer, InOther.BaseLayer );
    const TOptional<UInt32> ThisMipEnd    = RHICheckedAdd( BaseMip, MipCount );
    const TOptional<UInt32> OtherMipEnd   = RHICheckedAdd( InOther.BaseMip, InOther.MipCount );
    const TOptional<UInt32> ThisLayerEnd  = RHICheckedAdd( BaseLayer, LayerCount );
    const TOptional<UInt32> OtherLayerEnd = RHICheckedAdd( InOther.BaseLayer, InOther.LayerCount );

    if ( not ThisMipEnd or not OtherMipEnd or not ThisLayerEnd or not OtherLayerEnd )
    {
        return {};
    }

    const UInt32 MipEnd   = Maths::Min( *ThisMipEnd, *OtherMipEnd );
    const UInt32 LayerEnd = Maths::Min( *ThisLayerEnd, *OtherLayerEnd );
    if ( MipEnd <= MipBegin or LayerEnd <= LayerBegin )
    {
        return {};
    }
    return TOptional<FRHISubresourceRange>( FRHISubresourceRange{
        .BaseMip = MipBegin, .MipCount = MipEnd - MipBegin, .BaseLayer = LayerBegin, .LayerCount = LayerEnd - LayerBegin } );
}
