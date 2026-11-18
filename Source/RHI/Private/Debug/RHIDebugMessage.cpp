/**
 * @file RHIDebugMessage.cpp
 * @brief Implements structured RHI diagnostic context.
 */

#include <Debug/RHIDebugMessage.hpp>

Lumen::FRHIDebugMessage::FRHIDebugMessage ( const ERHIDebugSeverity InSeverity,
                                            const ERHIDebugCategory InCategory,
                                            const UInt32 InCode,
                                            const FStringView InMessage ) noexcept
    : Severity( InSeverity ), Category( InCategory ), Code( InCode ), Message( InMessage )
{
}

void Lumen::FRHIDebugMessage::AddObject ( const FRHIObjectId InId ) noexcept
{
    if ( InId.IsValid() )
    {
        ObjectIds.push_back( InId );
    }
}

void Lumen::FRHIDebugMessage::SetSubmissionContext ( const ERHIQueueType InQueue, const FRHITimelineValue InSubmission ) noexcept
{
    Queue.Emplace( InQueue );
    Submission.Emplace( InSubmission );
}

Lumen::Bool Lumen::FRHIDebugMessage::IsAtLeast ( const ERHIDebugSeverity InSeverity ) const noexcept
{
    return Severity >= InSeverity;
}
