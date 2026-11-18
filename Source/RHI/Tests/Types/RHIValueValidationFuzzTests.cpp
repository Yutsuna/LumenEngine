/**
 * @file RHIValueValidationFuzzTests.cpp
 * @brief Fuzzes checked arithmetic, format lookup, and descriptor boundaries.
 */

#include <Bindings/RHIBindingLayoutDesc.hpp>
#include <Commands/RHICopyDescs.hpp>
#include <Templates/Array.hpp>
#include <Tests/UnitTest.hpp>
#include <Types/RHICopyFootprint.hpp>
#include <Types/RHIFormatInfo.hpp>
#include <Utilities/RHIAlignment.hpp>
#include <Utilities/RHICheckedMath.hpp>
#include <Utilities/RHITestRandom.hpp>
#include <limits>

namespace Lumen
{
namespace
{

    LUMEN_TEST( RHIValueValidationFuzz, CheckedArithmeticMatchesOverflowFreeReferences )
    {
        Tests::FRHITestRandom Random( 0x434845434B4544ULL );
        constexpr UInt64 Maximum           = std::numeric_limits<UInt64>::max();
        const TArray<UInt64, 12U> Extremes = { 0ULL,  1ULL,  2ULL,          3ULL,           7ULL,           15ULL,
                                               16ULL, 31ULL, 0xFFFFFFFFULL, Maximum / 2ULL, Maximum - 1ULL, Maximum };
        for ( UInt32 Iteration = 0U; Iteration < 1024U; ++Iteration )
        {
            const UInt64 Left = Extremes[static_cast<USize>( Random.Next( Extremes.size() ) )];
            const UInt64 Right =
                Iteration % 3U == 0U ? Random.Next() : Extremes[static_cast<USize>( Random.Next( Extremes.size() ) )];
            const TOptional<UInt64> Sum = RHICheckedAdd( Left, Right );
            const Bool bSumFits         = Right <= Maximum - Left;
            LUMEN_CHECK_EQ( Sum.HasValue(), bSumFits );
            if ( Sum )
            {
                LUMEN_CHECK_EQ( *Sum, Left + Right );
            }

            const TOptional<UInt64> Product = RHICheckedMultiply( Left, Right );
            const Bool bProductFits         = Left == 0ULL or Right <= Maximum / Left;
            LUMEN_CHECK_EQ( Product.HasValue(), bProductFits );
            if ( Product )
            {
                LUMEN_CHECK_EQ( *Product, Left * Right );
            }

            const TOptional<UInt64> Quotient = RHICheckedDivideCeil( Left, Right );
            if ( Right == 0ULL )
            {
                LUMEN_CHECK_FALSE( Quotient.HasValue() );
            }
            else
            {
                LUMEN_CHECK_TRUE( Quotient.HasValue() );
                LUMEN_CHECK_EQ( *Quotient, Left / Right + ( Left % Right == 0ULL ? 0ULL : 1ULL ) );
            }

            const TOptional<UInt32> Cast = RHICheckedCast<UInt32>( Right );
            LUMEN_CHECK_EQ( Cast.HasValue(), Right <= std::numeric_limits<UInt32>::max() );
            if ( Cast )
            {
                LUMEN_CHECK_EQ( static_cast<UInt64>( *Cast ), Right );
            }

            const UInt64 Alignment          = 1ULL << Random.Next( 64ULL );
            const TOptional<UInt64> Aligned = RHIAlignUp( Left, Alignment );
            if ( Aligned )
            {
                LUMEN_CHECK_TRUE( *Aligned >= Left );
                LUMEN_CHECK_EQ( *Aligned % Alignment, 0ULL );
            }
            else
            {
                LUMEN_CHECK_TRUE( Left > Maximum - ( Alignment - 1ULL ) );
            }
        }
    }

    LUMEN_TEST( RHIValueValidationFuzz, FormatLookupAndFootprintsStayBounded )
    {
        Tests::FRHITestRandom Random( 0x464F4F545052494EULL );
        const TArray<UInt32, 14U> Dimensions = { 0U, 1U, 2U, 3U, 4U, 15U, 16U, 31U, 32U, 63U, 64U, 255U, 256U, 0xFFFFFFFFU };
        const TArray<UInt64, 12U> Alignments = {
            0ULL, 1ULL, 2ULL, 3ULL, 4ULL, 8ULL, 16ULL, 256ULL, 0x8000ULL, 0x8000000000000000ULL, 0xFFFFFFFFFFFFFFFFULL, 32ULL };
        for ( UInt32 Iteration = 0U; Iteration < 1024U; ++Iteration )
        {
            const ERHIFormat Format              = static_cast<ERHIFormat>( Random.Next( 512ULL ) );
            const TOptional<FRHIFormatInfo> Info = TryGetRHIFormatInfo( Format );
            if ( Info )
            {
                LUMEN_CHECK_EQ( GetRHIFormatInfo( Format ).Format, Info->Format );
            }
            FRHIExtent3D Extent;
            Extent.Width           = Dimensions[static_cast<USize>( Random.Next( Dimensions.size() ) )];
            Extent.Height          = Dimensions[static_cast<USize>( Random.Next( Dimensions.size() ) )];
            Extent.Depth           = Dimensions[static_cast<USize>( Random.Next( Dimensions.size() ) )];
            const UInt64 Offset    = Random.Next();
            const UInt64 Alignment = Alignments[static_cast<USize>( Random.Next( Alignments.size() ) )];
            const TExpected<FRHICopyFootprint, FRHIError> Footprint =
                ComputeRHICopyFootprint( Format, Extent, Offset, Alignment );
            if ( Footprint )
            {
                LUMEN_CHECK_EQ( Footprint->Offset, Offset );
                LUMEN_CHECK_TRUE( Footprint->RowPitch > 0ULL );
                LUMEN_CHECK_TRUE( Footprint->SlicePitch >= Footprint->RowPitch );
                LUMEN_CHECK_TRUE( Footprint->TotalSize >= Footprint->SlicePitch );
                LUMEN_CHECK_EQ( Footprint->SliceCount, Extent.Depth );
                LUMEN_CHECK_EQ( Footprint->RowPitch % Alignment, 0ULL );
            }
            else
            {
                LUMEN_CHECK_EQ( Footprint.Error().Condition, ERHIErrorCondition::Resource );
            }
        }
    }

    LUMEN_TEST( RHIValueValidationFuzz, CopyAndBindingDescriptorsRejectOnlyInvalidBoundaries )
    {
        Tests::FRHITestRandom Random( 0x44455343524E54ULL );
        for ( UInt32 Iteration = 0U; Iteration < 1024U; ++Iteration )
        {
            FRHIBufferCopy Copy;
            Copy.SourceOffset                      = Random.Next();
            Copy.DestinationOffset                 = Random.Next();
            Copy.Size                              = Random.Next();
            const UInt64 SourceSize                = Random.Next();
            const UInt64 DestinationSize           = Random.Next();
            const Bool bSameBuffer                 = Random.Next( 2ULL ) == 0ULL;
            const TExpected<void, FRHIError> Valid = Copy.Validate( SourceSize, DestinationSize, bSameBuffer );
            if ( not Valid )
            {
                LUMEN_CHECK_EQ( Valid.Error().Code, ERHIErrorCode::InvalidArgument );
                LUMEN_CHECK_EQ( Valid.Error().Condition, ERHIErrorCondition::Command );
            }

            FRHIBindingLayoutDesc Layout;
            FRHIBindingLayoutEntry Entry;
            Entry.Slot   = static_cast<UInt32>( Random.Next() );
            Entry.Count  = static_cast<UInt32>( Random.Next() );
            Entry.Stages = static_cast<ERHIShaderStage>( Random.Next( 64ULL ) );
            Layout.Entries.push_back( Entry );
            const TExpected<void, FRHIError> LayoutValid = Layout.Validate();
            LUMEN_CHECK_EQ( LayoutValid.HasValue(), Entry.Count != 0U and Entry.Stages != ERHIShaderStage::None );
            if ( LayoutValid )
            {
                /** NOTE: Independent construction detects padding-dependent hash nondeterminism. */
                FRHIBindingLayoutDesc IndependentLayout;
                FRHIBindingLayoutEntry IndependentEntry;
                IndependentEntry.Slot   = Entry.Slot;
                IndependentEntry.Count  = Entry.Count;
                IndependentEntry.Stages = Entry.Stages;
                IndependentLayout.Entries.push_back( IndependentEntry );
                LUMEN_CHECK_EQ( Layout.GetHash(), IndependentLayout.GetHash() );

                const FRHIBindingLayoutDesc CopiedLayout = Layout;
                LUMEN_CHECK_EQ( Layout.GetHash(), CopiedLayout.GetHash() );
            }
        }
    }

} // namespace
} // namespace Lumen
