/**
 * @file ReplicationTypes.inl
 * @brief Inline implementations of replication value types.
 */

#pragma once

#include <Replication/ReplicationTypes.hpp>

#include <Templates/Unexpected.hpp>

inline constexpr Lumen::FPersistentActorId::FPersistentActorId ( const UInt64 InValue ) noexcept : Value( InValue )
{
}

inline constexpr Lumen::Bool Lumen::FPersistentActorId::IsValid () const noexcept
{
    return Value != 0ULL;
}

inline constexpr Lumen::UInt64 Lumen::FPersistentActorId::GetValue () const noexcept
{
    return Value;
}

inline constexpr Lumen::USize Lumen::ReplicationChannelIndex ( const EReplicationDirection InDirection,
                                                               const EReplicationChannel InChannel ) noexcept
{
    return ( static_cast<USize>( InDirection ) * 2ULL ) + static_cast<USize>( InChannel );
}

inline void Lumen::FDirtyPropertyMask::Mark ( const EActorReplicationProperty InProperty ) noexcept
{
    Bits |= 1U << static_cast<UInt32>( InProperty );
}

inline void Lumen::FDirtyPropertyMask::Unmark ( const EActorReplicationProperty InProperty ) noexcept
{
    Bits &= ~( 1U << static_cast<UInt32>( InProperty ) );
}

inline Lumen::Bool Lumen::FDirtyPropertyMask::Has ( const EActorReplicationProperty InProperty ) const noexcept
{
    return ( Bits & ( 1U << static_cast<UInt32>( InProperty ) ) ) != 0U;
}

inline void Lumen::FDirtyPropertyMask::Clear () noexcept
{
    Bits = 0U;
}

inline Lumen::Bool Lumen::FDirtyPropertyMask::IsEmpty () const noexcept
{
    return Bits == 0U;
}

inline Lumen::UInt32 Lumen::FDirtyPropertyMask::GetBits () const noexcept
{
    return Bits;
}

inline Lumen::TExpected<void, Lumen::EReplicationError> Lumen::FReplicationConfig::Validate () const noexcept
{
    if ( MaxActors == 0ULL or MaxRecordsPerFrame == 0ULL or MaxPayloadBytes == 0ULL or ResendWindow == 0ULL or
         MaxBufferedFrames == 0ULL or MaxPendingControlRecords == 0ULL or MaxPendingParents == 0ULL or MaxPendingInputs == 0ULL or
         MaxResendAttempts == 0U or FullSnapshotTickThreshold == 0U or FullSnapshotCooldownTicks == 0U or
         MaxPendingParentTicks == 0U )
    {
        return TUnexpected<EReplicationError>( EReplicationError::InvalidCapacity );
    }
    const USize WorstCaseRecords = MaxActors * MaxReplicationRecordsPerActor;
    if ( MaxRecordsPerFrame < WorstCaseRecords )
    {
        return TUnexpected<EReplicationError>( EReplicationError::InvalidCapacity );
    }
    if ( MaxPayloadBytes < WorstCaseRecords * MaxReplicationRecordBytes )
    {
        return TUnexpected<EReplicationError>( EReplicationError::InvalidCapacity );
    }
    return {};
}
