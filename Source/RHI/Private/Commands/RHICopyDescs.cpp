/**
 * @file RHICopyDescs.cpp
 * @brief Implements bounded copy descriptor validation.
 */

#include <Commands/RHICopyDescs.hpp>
#include <Types/RHICopyFootprint.hpp>
#include <Utilities/RHIAlignment.hpp>
#include <Utilities/RHICheckedMath.hpp>
namespace
{
Lumen::TExpected<void, Lumen::FRHIError> InvalidCopy ( Lumen::FStringView InMessage ) noexcept
{
    return Lumen::TUnexpected<Lumen::FRHIError>( Lumen::MakeRHIInvalidArgument( Lumen::ERHIErrorCondition::Command, InMessage ) );
}
Lumen::Bool IsAspectValid ( const Lumen::FRHIFormatInfo &InInfo, const Lumen::ERHIFormatAspect InAspect ) noexcept
{
    const Lumen::UInt32 Requested = static_cast<Lumen::UInt32>( InAspect );
    return Requested != 0U and ( static_cast<Lumen::UInt32>( InInfo.Aspects ) & Requested ) == Requested;
}
} // namespace
Lumen::TExpected<void, Lumen::FRHIError> Lumen::FRHIBufferCopy::Validate () const noexcept
{
    if ( Size == 0ULL or not RHICheckedAdd( SourceOffset, Size ) or not RHICheckedAdd( DestinationOffset, Size ) )
    {
        return InvalidCopy( "Buffer copy is empty or overflows" );
    }
    return {};
}
Lumen::TExpected<void, Lumen::FRHIError> Lumen::FRHIBufferCopy::Validate ( const UInt64 InSourceSize,
                                                                           const UInt64 InDestinationSize,
                                                                           const Bool bSameBuffer ) const noexcept
{
    auto Basic = Validate();
    if ( not Basic )
    {
        return Basic;
    }
    if ( SourceOffset + Size > InSourceSize or DestinationOffset + Size > InDestinationSize or
         ( bSameBuffer and RHIRangesOverlap( SourceOffset, Size, DestinationOffset, Size ) ) )
    {
        return InvalidCopy( "Buffer copy is out of bounds or overlaps" );
    }
    return {};
}
Lumen::TExpected<void, Lumen::FRHIError> Lumen::FRHITextureCopy::Validate ( const ERHIFormat InFormat ) const noexcept
{
    const auto Info = TryGetRHIFormatInfo( InFormat );
    if ( not Info or not IsAspectValid( *Info, Aspect ) or not SourceOffset.IsNonNegative() or
         not DestinationOffset.IsNonNegative() or not Extent.IsValid() )
    {
        return InvalidCopy( "Texture copy has invalid offsets, extent, or aspects" );
    }
    return {};
}
Lumen::TExpected<void, Lumen::FRHIError> Lumen::FRHIBufferTextureCopy::Validate ( const ERHIFormat InFormat ) const noexcept
{
    const auto Info = TryGetRHIFormatInfo( InFormat );
    if ( not Info or not IsAspectValid( *Info, Aspect ) or not TextureOffset.IsNonNegative() or not Extent.IsValid() )
    {
        return InvalidCopy( "Buffer-texture copy has invalid offsets, extent, or aspects" );
    }
    auto Footprint = ComputeRHICopyFootprint( InFormat, Extent, BufferOffset );
    if ( not Footprint )
    {
        return TUnexpected<FRHIError>( Footprint.Error() );
    }
    const UInt64 EffectiveRowPitch = RowPitch == 0ULL ? Footprint->RowPitch : RowPitch;
    const auto MinimumSlicePitch   = RHICheckedMultiply( EffectiveRowPitch, static_cast<UInt64>( Footprint->RowCount ) );
    UInt64 EffectiveSlicePitch     = SlicePitch;
    if ( SlicePitch == 0ULL and MinimumSlicePitch )
    {
        EffectiveSlicePitch = *MinimumSlicePitch;
    }
    const auto TotalSize = RHICheckedMultiply( EffectiveSlicePitch, static_cast<UInt64>( Footprint->SliceCount ) );
    const auto End       = TotalSize ? RHICheckedAdd( BufferOffset, *TotalSize ) : TOptional<UInt64>{};
    if ( EffectiveRowPitch < Footprint->RowPitch or not MinimumSlicePitch or EffectiveSlicePitch < *MinimumSlicePitch or not End )
    {
        return InvalidCopy( "Buffer-texture pitches or total size are invalid" );
    }
    return {};
}
