/**
 * @file RHIDebugMessageTests.cpp
 * @brief Tests owned debug text and structured diagnostic context.
 */

#include <Debug/RHIDebugMessage.hpp>
#include <Tests/UnitTest.hpp>

namespace Lumen
{
namespace
{

    LUMEN_TEST( RHIDebugMessage, OwnsTextAndFiltersSeverity )
    {
        FString Text = "owned diagnostic";
        FRHIDebugMessage Message( ERHIDebugSeverity::Warning, ERHIDebugCategory::Validation, 17U, Text );
        Text[0] = 'X';
        LUMEN_CHECK_TRUE( Message.Message == "owned diagnostic" );
        LUMEN_CHECK_TRUE( Message.IsAtLeast( ERHIDebugSeverity::Info ) );
        LUMEN_CHECK_TRUE( Message.IsAtLeast( ERHIDebugSeverity::Warning ) );
        LUMEN_CHECK_FALSE( Message.IsAtLeast( ERHIDebugSeverity::Error ) );
        LUMEN_CHECK_EQ( Message.Code, 17U );
    }

    LUMEN_TEST( RHIDebugMessage, RetainsObjectAndSubmissionContext )
    {
        FRHIDebugMessage Message;
        Message.AddObject( {} );
        LUMEN_CHECK_TRUE( Message.ObjectIds.empty() );
        Message.AddObject( FRHIObjectId{ 42ULL } );
        Message.SetSubmissionContext( ERHIQueueType::Graphics, { 9ULL } );
        LUMEN_CHECK_EQ( Message.ObjectIds.size(), 1U );
        LUMEN_CHECK_EQ( Message.ObjectIds[0].GetValue(), 42ULL );
        LUMEN_CHECK_TRUE( Message.Queue.HasValue() );
        LUMEN_CHECK_EQ( *Message.Queue, ERHIQueueType::Graphics );
        LUMEN_CHECK_TRUE( Message.Submission.HasValue() );
        LUMEN_CHECK_EQ( Message.Submission->Value, 9ULL );
    }

} // namespace
} // namespace Lumen
