/**
 * @file ReplicationFrame.cpp
 * @brief Out-of-line reservation of the replication codec scratch storage.
 */

#include <Replication/ReplicationFrame.hpp>

void Lumen::FReplicationFrameBuilder::Reserve ( const USize InMaxPayloadBytes, const USize InMaxRecordsPerFrame ) noexcept
{
    if ( Buffer.capacity() < InMaxPayloadBytes )
    {
        Buffer.reserve( InMaxPayloadBytes );
    }
    if ( Records.capacity() < InMaxRecordsPerFrame )
    {
        Records.reserve( InMaxRecordsPerFrame );
    }
}
