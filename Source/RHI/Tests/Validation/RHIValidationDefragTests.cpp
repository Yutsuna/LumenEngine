/**
 * @file RHIValidationDefragTests.cpp
 * @brief Tests defragmentation plan validation observed through a validated device.
 */
#include "Validation/RHIValidationReporter.hpp"
#include "Validation/RHIValidationTestUtils.hpp"
#include <RHI/Memory/IRHIDefragmenter.hpp>
#include <Tests/UnitTest.hpp>

namespace Lumen
{

namespace RHI
{
    namespace
    {
        LUMEN_TEST( RHIValidationDefrag, RejectsInvalidPlansAndReportsDiagnostic )
        {
            Tests::FRecordingDebugSink Sink;
            TSharedRef<IRHIDevice> Device             = Tests::Validation::MakeDevice( Sink );
            TSharedRef<IRHIDefragmenter> Defragmenter = *Device->CreateDefragmenter();
            FRHIDefragPlan Empty                      = *Defragmenter->BuildPlan( {} );
            LUMEN_CHECK_TRUE( Defragmenter->Execute( Empty ).HasValue() );
            LUMEN_CHECK_EQ( Sink.GetCount(), 0U );
            FRHIAllocation Source;
            Source.Id        = FRHIAllocationId{ 1ULL };
            Source.Size      = 64ULL;
            Source.Alignment = 16ULL;
            FRHIDefragPlan Invalid;
            Invalid.Moves.push_back( { .Source = Source, .Destination = {} } );
            LUMEN_CHECK_HAS_ERROR( Invalid.Validate() );
            const TExpected<void, FRHIError> Executed = Defragmenter->Execute( Invalid );
            LUMEN_CHECK_HAS_ERROR( Executed );
            LUMEN_CHECK_EQ( Executed.Error().Code, ERHIErrorCode::ValidationFailed );
            LUMEN_CHECK_TRUE( Sink.ContainsCode( static_cast<UInt32>( ERHIValidationMessageCode::InvalidDefragPlan ) ) );
        }
    } // namespace
} // namespace RHI

} // namespace Lumen
