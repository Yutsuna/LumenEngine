/**
 * @file RHICopyFootprint.cpp
 * @brief Implements overflow-safe linear copy footprint calculation.
 */

#include <Types/RHICopyFootprint.hpp>
#include <Types/RHIFormatInfo.hpp>
#include <Utilities/RHIAlignment.hpp>
#include <Utilities/RHICheckedMath.hpp>

Lumen::TExpected<Lumen::FRHICopyFootprint, Lumen::FRHIError> Lumen::ComputeRHICopyFootprint (
    const ERHIFormat InFormat, const FRHIExtent3D InExtent, const UInt64 InOffset, const UInt64 InRowAlignment ) noexcept
{
    const TOptional<FRHIFormatInfo> Info = TryGetRHIFormatInfo( InFormat );
    if ( not Info or InFormat == ERHIFormat::Unknown or not InExtent.IsValid() or not RHIIsPowerOfTwo( InRowAlignment ) )
    {
        return TUnexpected<FRHIError>( MakeRHIInvalidArgument(
            ERHIErrorCondition::Resource, "Copy footprint requires a known format, extent, and valid alignment" ) );
    }

    const TOptional<UInt32> BlocksX = RHICheckedDivideCeil( InExtent.Width, static_cast<UInt32>( Info->BlockWidth ) );
    const TOptional<UInt32> BlocksY = RHICheckedDivideCeil( InExtent.Height, static_cast<UInt32>( Info->BlockHeight ) );
    if ( not BlocksX or not BlocksY )
    {
        return TUnexpected<FRHIError>( MakeRHIInvalidResource( "Copy block count overflow" ) );
    }

    const TOptional<UInt64> TightPitch =
        RHICheckedMultiply( static_cast<UInt64>( *BlocksX ), static_cast<UInt64>( Info->BytesPerBlock ) );
    if ( not TightPitch )
    {
        return TUnexpected<FRHIError>( MakeRHIInvalidResource( "Copy row pitch overflow" ) );
    }
    const TOptional<UInt64> RowPitch = RHIAlignUp( *TightPitch, InRowAlignment );
    const TOptional<UInt64> SlicePitch =
        RowPitch ? RHICheckedMultiply( *RowPitch, static_cast<UInt64>( *BlocksY ) ) : TOptional<UInt64>{};
    const TOptional<UInt64> TotalSize =
        SlicePitch ? RHICheckedMultiply( *SlicePitch, static_cast<UInt64>( InExtent.Depth ) ) : TOptional<UInt64>{};
    if ( not RowPitch or not SlicePitch or not TotalSize )
    {
        return TUnexpected<FRHIError>( MakeRHIInvalidResource( "Copy footprint size overflow" ) );
    }

    return FRHICopyFootprint{
        .Offset     = InOffset,
        .RowPitch   = *RowPitch,
        .SlicePitch = *SlicePitch,
        .TotalSize  = *TotalSize,
        .RowCount   = *BlocksY,
        .SliceCount = InExtent.Depth,
    };
}
