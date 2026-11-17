/**
 * @file RHIQueryPoolDesc.cpp
 * @brief Implements query result layout and pool descriptor validation.
 */

#include <Queries/RHIQueryPoolDesc.hpp>

Lumen::UInt32 Lumen::GetRHIQueryValueCount ( const ERHIQueryType InType ) noexcept
{
    switch ( InType )
    {
    case ERHIQueryType::Occlusion:
    case ERHIQueryType::Timestamp:
        return 1U;
    case ERHIQueryType::PipelineStatistics:
        return 11U;
    default:
        return 0U;
    }
}

Lumen::UInt32 Lumen::GetRHIQueryResultSize ( const ERHIQueryType InType ) noexcept
{
    return GetRHIQueryValueCount( InType ) * static_cast<UInt32>( sizeof( UInt64 ) );
}

Lumen::TExpected<void, Lumen::FRHIError> Lumen::FRHIQueryPoolDesc::Validate () const noexcept
{
    const Bool bQueueCompatible =
        QueueType == ERHIQueueType::Graphics or ( Type == ERHIQueryType::Timestamp and QueueType == ERHIQueueType::Compute );
    if ( Type >= ERHIQueryType::Count or Count == 0U or QueueType >= ERHIQueueType::Count or not bQueueCompatible )
    {
        return TUnexpected<FRHIError>( MakeRHIInvalidQuery( "Invalid query type, count, or queue compatibility" ) );
    }
    return {};
}

Lumen::UInt32 Lumen::FRHIQueryPoolDesc::GetResultSize () const noexcept
{
    return GetRHIQueryResultSize( Type );
}
