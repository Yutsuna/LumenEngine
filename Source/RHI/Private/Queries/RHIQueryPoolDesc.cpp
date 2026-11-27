/**
 * @file RHIQueryPoolDesc.cpp
 * @brief Implements query result layout and pool descriptor validation.
 */

#include <RHI/Queries/RHIQueryPoolDesc.hpp>

Lumen::TOptional<Lumen::UInt32> Lumen::RHI::GetRHIQueryValueCount ( const ERHIQueryType InType ) noexcept
{
    switch ( InType )
    {
    case ERHIQueryType::Occlusion:
    case ERHIQueryType::Timestamp:
        return TOptional<UInt32>( 1U );
    case ERHIQueryType::PipelineStatistics:
        return TOptional<UInt32>( 11U );
    default:
        return {};
    }
}

Lumen::TOptional<Lumen::UInt32> Lumen::RHI::GetRHIQueryResultSize ( const ERHIQueryType InType ) noexcept
{
    return GetRHIQueryValueCount( InType ).Transform( [] ( const UInt32 InValueCount ) noexcept
    {
        return InValueCount * static_cast<UInt32>( sizeof( UInt64 ) );
    } );
}

Lumen::RHI::FRHIExpected Lumen::RHI::FRHIQueryPoolDesc::Validate () const noexcept
{
    const Bool bQueueCompatible =
        QueueType == ERHIQueueType::Graphics or ( Type == ERHIQueryType::Timestamp and QueueType == ERHIQueueType::Compute );
    if ( Type >= ERHIQueryType::Count or Count == 0U or QueueType >= ERHIQueueType::Count or not bQueueCompatible )
    {
        return TUnexpected<FRHIError>( MakeRHIInvalidQuery( "Invalid query type, count, or queue compatibility" ) );
    }
    return {};
}

Lumen::UInt32 Lumen::RHI::FRHIQueryPoolDesc::GetResultSize () const noexcept
{
    return GetRHIQueryResultSize( Type ).ValueOr( 0U );
}
