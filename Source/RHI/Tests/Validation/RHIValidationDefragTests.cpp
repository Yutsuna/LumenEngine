/**
 * @file RHIValidationDefragTests.cpp
 * @brief Tests defragmenter transparency and invalid-move preservation under validation.
 */
#include "Validation/RHIValidationTestUtils.hpp"
#include <Memory/IRHIDefragmenter.hpp>
#include <Tests/UnitTest.hpp>

namespace Lumen
{
namespace
{
    LUMEN_TEST( RHIValidationDefrag, PreservesEmptyPlansAndRejectsInvalidMoves )
    {
        Tests::FRecordingDebugSink Sink;
        TSharedRef<IRHIDevice> Device             = Tests::Validation::MakeDevice( Sink );
        TSharedRef<IRHIDefragmenter> Defragmenter = *Device->CreateDefragmenter();
        FRHIDefragPlan Empty                      = *Defragmenter->BuildPlan( {} );
        LUMEN_CHECK_TRUE( Defragmenter->Execute( Empty ).HasValue() );
        FRHIAllocation Source;
        Source.Id        = FRHIAllocationId{ 1ULL };
        Source.Size      = 64ULL;
        Source.Alignment = 16ULL;
        FRHIDefragPlan Invalid;
        Invalid.Moves.push_back( { .Source = Source, .Destination = {} } );
        LUMEN_CHECK_HAS_ERROR( Invalid.Validate() );
        LUMEN_CHECK_HAS_ERROR( Defragmenter->Execute( Invalid ) );
        LUMEN_CHECK_EQ( Sink.GetCount(), 0U );
    }
} // namespace
} // namespace Lumen
