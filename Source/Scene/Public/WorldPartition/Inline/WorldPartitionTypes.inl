/**
 * @file WorldPartitionTypes.inl
 * @brief Inline implementations of world partition value types.
 */

#pragma once

#include <WorldPartition/WorldPartitionTypes.hpp>

#include <Templates/Unexpected.hpp>

inline constexpr Lumen::FWorldCellId::FWorldCellId ( const UInt64 InValue ) noexcept : Value( InValue )
{
}

inline constexpr Lumen::Bool Lumen::FWorldCellId::IsValid () const noexcept
{
    return Value != 0ULL;
}

inline constexpr Lumen::UInt64 Lumen::FWorldCellId::GetValue () const noexcept
{
    return Value;
}

inline Lumen::TExpected<void, Lumen::EWorldPartitionError> Lumen::FWorldPartitionConfig::Validate () const noexcept
{
    const Bool bAllZero = MaxCells == 0ULL and MaxAssignedActors == 0ULL and MaxJournalRecords == 0ULL;
    const Bool bAllSet  = MaxCells != 0ULL and MaxAssignedActors != 0ULL and MaxJournalRecords != 0ULL;
    if ( not bAllZero and not bAllSet )
    {
        return TUnexpected<EWorldPartitionError>( EWorldPartitionError::InvalidCapacity );
    }
    return {};
}
