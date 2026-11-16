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
    const FRHIFormatInfo &Info = GetRHIFormatInfo( InFormat );
    if ( InFormat == ERHIFormat::Unknown or not InExtent.IsValid() or not RHIIsPowerOfTwo( InRowAlignment ) )
    {
        return TUnexpected<FRHIError>( MakeRHIInvalidArgument(
            ERHIErrorCondition::Resource, "Copy footprint requires a known format, extent, and valid alignment" ) );
    }

    const TOptional<UInt32> BlocksX = RHICheckedDivideCeil( InExtent.Width, static_cast<UInt32>( Info.BlockWidth ) );
    const TOptional<UInt32> BlocksY = RHICheckedDivideCeil( InExtent.Height, static_cast<UInt32>( Info.BlockHeight ) );
    if ( not BlocksX or not BlocksY )
    {
        return TUnexpected<FRHIError>( MakeRHIInvalidArgument( ERHIErrorCondition::Resource, "Copy block count overflow" ) );
    }

    const auto TightPitch = RHICheckedMultiply( static_cast<UInt64>( *BlocksX ), static_cast<UInt64>( Info.BytesPerBlock ) );
    if ( not TightPitch )
    {
        return TUnexpected<FRHIError>( MakeRHIInvalidArgument( ERHIErrorCondition::Resource, "Copy row pitch overflow" ) );
    }
    const auto RowPitch   = RHIAlignUp( *TightPitch, InRowAlignment );
    const auto SlicePitch = RowPitch ? RHICheckedMultiply( *RowPitch, static_cast<UInt64>( *BlocksY ) ) : TOptional<UInt64>{};
    const auto TotalSize =
        SlicePitch ? RHICheckedMultiply( *SlicePitch, static_cast<UInt64>( InExtent.Depth ) ) : TOptional<UInt64>{};
    if ( not RowPitch or not SlicePitch or not TotalSize )
    {
        return TUnexpected<FRHIError>( MakeRHIInvalidArgument( ERHIErrorCondition::Resource, "Copy footprint size overflow" ) );
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
