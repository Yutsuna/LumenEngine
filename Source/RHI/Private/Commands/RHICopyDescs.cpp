/**
 * @file RHICopyDescs.cpp
 * @brief Implements bounded copy descriptor validation.
 */

#include <RHI/Commands/RHICopyDescs.hpp>
#include <RHI/Types/RHICopyFootprint.hpp>
#include <RHI/Utilities/RHIAlignment.hpp>
#include <RHI/Utilities/RHICheckedMath.hpp>

/**
 * Private
 */

namespace
{

Lumen::TExpected<void, Lumen::RHI::FRHIError> InvalidCopy ( Lumen::FStringView InMessage ) noexcept
{
    return Lumen::TUnexpected<Lumen::RHI::FRHIError>(
        Lumen::RHI::MakeRHIInvalidArgument( Lumen::RHI::ERHIErrorCondition::Command, InMessage ) );
}

Lumen::Bool IsAspectValid ( const Lumen::RHI::FRHIFormatInfo &InInfo, const Lumen::RHI::ERHIFormatAspect InAspect ) noexcept
{
    const Lumen::UInt32 Requested = static_cast<Lumen::UInt32>( InAspect );
    return Requested != 0U and ( static_cast<Lumen::UInt32>( InInfo.Aspects ) & Requested ) == Requested;
}

} // namespace

/**
 * Public
 */

Lumen::TExpected<void, Lumen::RHI::FRHIError> Lumen::RHI::FRHIBufferCopy::Validate () const noexcept
{
    if ( Size == 0ULL or not RHICheckedAdd( SourceOffset, Size ) or not RHICheckedAdd( DestinationOffset, Size ) )
    {
        return InvalidCopy( "Buffer copy is empty or overflows" );
    }
    return {};
}

Lumen::TExpected<void, Lumen::RHI::FRHIError> Lumen::RHI::FRHIBufferCopy::Validate ( const UInt64 InSourceSize,
                                                                                     const UInt64 InDestinationSize,
                                                                                     const Bool bSameBuffer ) const noexcept
{
    return Validate().AndThen( [this, InSourceSize, InDestinationSize, bSameBuffer] () noexcept -> TExpected<void, FRHIError>
    {
        if ( SourceOffset + Size > InSourceSize or DestinationOffset + Size > InDestinationSize or
             ( bSameBuffer and RHIRangesOverlap( SourceOffset, Size, DestinationOffset, Size ) ) )
        {
            return InvalidCopy( "Buffer copy is out of bounds or overlaps" );
        }
        return {};
    } );
}

Lumen::TExpected<void, Lumen::RHI::FRHIError> Lumen::RHI::FRHITextureCopy::Validate ( const ERHIFormat InFormat ) const noexcept
{
    const TOptional<FRHIFormatInfo> Info = TryGetRHIFormatInfo( InFormat );
    if ( not Info or not IsAspectValid( *Info, Aspect ) or not SourceOffset.IsNonNegative() or
         not DestinationOffset.IsNonNegative() or not Extent.IsValid() )
    {
        return InvalidCopy( "Texture copy has invalid offsets, extent, or aspects" );
    }
    return {};
}

Lumen::TExpected<void, Lumen::RHI::FRHIError>
Lumen::RHI::FRHIBufferTextureCopy::Validate ( const ERHIFormat InFormat ) const noexcept
{
    const TOptional<FRHIFormatInfo> Info = TryGetRHIFormatInfo( InFormat );
    if ( not Info or not IsAspectValid( *Info, Aspect ) or not TextureOffset.IsNonNegative() or not Extent.IsValid() )
    {
        return InvalidCopy( "Buffer-texture copy has invalid offsets, extent, or aspects" );
    }
    return ComputeRHICopyFootprint( InFormat, Extent, BufferOffset )
        .AndThen( [this] ( const FRHICopyFootprint &InFootprint ) noexcept -> TExpected<void, FRHIError>
    {
        const UInt64 EffectiveRowPitch = RowPitch == 0ULL ? InFootprint.RowPitch : RowPitch;
        const TOptional<UInt64> MinimumSlicePitch =
            RHICheckedMultiply( EffectiveRowPitch, static_cast<UInt64>( InFootprint.RowCount ) );
        UInt64 EffectiveSlicePitch = SlicePitch;

        if ( SlicePitch == 0ULL and MinimumSlicePitch )
        {
            EffectiveSlicePitch = *MinimumSlicePitch;
        }

        const TOptional<UInt64> TotalSize =
            RHICheckedMultiply( EffectiveSlicePitch, static_cast<UInt64>( InFootprint.SliceCount ) );
        const TOptional<UInt64> End = TotalSize ? RHICheckedAdd( BufferOffset, *TotalSize ) : TOptional<UInt64>{};

        if ( EffectiveRowPitch < InFootprint.RowPitch or not MinimumSlicePitch or EffectiveSlicePitch < *MinimumSlicePitch or
             not End )
        {
            return InvalidCopy( "Buffer-texture pitches or total size are invalid" );
        }
        return {};
    } );
}
