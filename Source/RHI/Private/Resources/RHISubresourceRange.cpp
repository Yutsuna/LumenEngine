/**
 * @file RHISubresourceRange.cpp
 * @brief Implements subresource range containment and intersection.
 */
#include <Resources/RHISubresourceRange.hpp>
#include <Utilities/RHICheckedMath.hpp>
#include <algorithm>
Lumen::Bool Lumen::FRHISubresourceRange::IsContainedIn ( const UInt32 InMipCount, const UInt32 InLayerCount ) const noexcept
{
    const auto MipEnd   = RHICheckedAdd( BaseMip, MipCount );
    const auto LayerEnd = RHICheckedAdd( BaseLayer, LayerCount );
    return IsValid() and MipEnd and LayerEnd and *MipEnd <= InMipCount and *LayerEnd <= InLayerCount;
}
Lumen::TOptional<Lumen::FRHISubresourceRange>
Lumen::FRHISubresourceRange::Intersect ( const FRHISubresourceRange &InOther ) const noexcept
{
    const UInt32 MipBegin    = std::max( BaseMip, InOther.BaseMip );
    const UInt32 LayerBegin  = std::max( BaseLayer, InOther.BaseLayer );
    const auto ThisMipEnd    = RHICheckedAdd( BaseMip, MipCount );
    const auto OtherMipEnd   = RHICheckedAdd( InOther.BaseMip, InOther.MipCount );
    const auto ThisLayerEnd  = RHICheckedAdd( BaseLayer, LayerCount );
    const auto OtherLayerEnd = RHICheckedAdd( InOther.BaseLayer, InOther.LayerCount );
    if ( not ThisMipEnd or not OtherMipEnd or not ThisLayerEnd or not OtherLayerEnd )
    {
        return {};
    }
    const UInt32 MipEnd   = std::min( *ThisMipEnd, *OtherMipEnd );
    const UInt32 LayerEnd = std::min( *ThisLayerEnd, *OtherLayerEnd );
    if ( MipEnd <= MipBegin or LayerEnd <= LayerBegin )
    {
        return {};
    }
    return TOptional<FRHISubresourceRange>(
        FRHISubresourceRange{ MipBegin, MipEnd - MipBegin, LayerBegin, LayerEnd - LayerBegin } );
}
