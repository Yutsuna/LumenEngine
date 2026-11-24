/**
 * @file NullQueries.cpp
 * @brief Implements deterministic Null query recording and readback.
 */

#include <HAL/LockGuard.hpp>
#include <Queries/NullQueries.hpp>
#include <Templates/Hash.hpp>

Lumen::RHI::Tests::FNullQueryPool::FNullQueryPool ( const FRHIQueryPoolDesc &InDesc,
                                                    TSharedPtr<FNullFaultPlan> InFaultPlan,
                                                    const FRHIObjectId InId ) noexcept
    : Identity( InId, FHash::TypeHash<FNullQueryPool>() ), Desc( InDesc ), FaultPlan( std::move( InFaultPlan ) ),
      Entries( InDesc.Count )
{
}

LUMEN_RHI_OBJECT_FORWARDERS( RHI::Tests, FNullQueryPool )

const Lumen::RHI::FRHIQueryPoolDesc &Lumen::RHI::Tests::FNullQueryPool::GetDesc () const noexcept
{
    return Desc;
}

void Lumen::RHI::Tests::FNullQueryPool::Reset () noexcept
{
    HAL::TLockGuard<HAL::FPlatformRecursiveSharedMutex> Lock( Mutex );
    for ( auto &Entry : Entries )
    {
        Entry = {};
    }
}

Lumen::RHI::FRHIExpected Lumen::RHI::Tests::FNullQueryPool::ReadResults ( const UInt32 InFirstQuery,
                                                                          const UInt32 InQueryCount,
                                                                          const TSpan<UInt64> InDestination,
                                                                          const FRHITimelineValue InCompleted ) const noexcept
{
    if ( FaultPlan )
    {
        if ( TOptional<FRHIError> Error = FaultPlan->ShouldFail( "ReadResults" ) )
        {
            return TUnexpected<FRHIError>( *Error );
        }
    }
    const UInt32 ValueCount = GetRHIQueryValueCount( Desc.Type ).ValueOr( 0U );
    if ( InFirstQuery > Desc.Count or InQueryCount > Desc.Count - InFirstQuery or
         InDestination.size() < static_cast<USize>( InQueryCount ) * ValueCount )
    {
        return TUnexpected<FRHIError>( MakeRHIInvalidQuery( "Query readback range or destination is invalid" ) );
    }
    HAL::TLockGuard<HAL::FPlatformRecursiveSharedMutex> Lock( Mutex );
    for ( UInt32 QueryOffset = 0U; QueryOffset < InQueryCount; ++QueryOffset )
    {
        const FEntry &Entry = Entries[InFirstQuery + QueryOffset];
        if ( not Entry.bAvailable or not IsRHICompletionReached( InCompleted, Entry.Availability ) )
        {
            return TUnexpected<FRHIError>(
                FRHIError( ERHIErrorCode::InvalidState, ERHIErrorCondition::Query, "Requested query result is unavailable" ) );
        }
        for ( UInt32 ValueIndex = 0U; ValueIndex < ValueCount; ++ValueIndex )
        {
            InDestination[static_cast<USize>( QueryOffset ) * ValueCount + ValueIndex] = Entry.Values[ValueIndex];
        }
    }
    return {};
}

Lumen::RHI::FRHIExpected Lumen::RHI::Tests::FNullQueryPool::Begin ( const UInt32 InIndex ) noexcept
{
    if ( TOptional<FRHIError> Error = CheckFault( "BeginQuery" ) )
    {
        return TUnexpected<FRHIError>( *Error );
    }
    HAL::TLockGuard<HAL::FPlatformRecursiveSharedMutex> Lock( Mutex );
    if ( InIndex >= Entries.size() )
    {
        return TUnexpected<FRHIError>( MakeRHIInvalidQuery( "Query index is out of range" ) );
    }
    FEntry &Entry = Entries[InIndex];
    if ( Entry.bActive )
    {
        return TUnexpected<FRHIError>(
            FRHIError( ERHIErrorCode::InvalidState, ERHIErrorCondition::Query, "Query is already active" ) );
    }
    Entry         = {};
    Entry.bActive = true;
    return {};
}

Lumen::RHI::FRHIExpected Lumen::RHI::Tests::FNullQueryPool::End ( const UInt32 InIndex ) noexcept
{
    if ( TOptional<FRHIError> Error = CheckFault( "EndQuery" ) )
    {
        return TUnexpected<FRHIError>( *Error );
    }
    HAL::TLockGuard<HAL::FPlatformRecursiveSharedMutex> Lock( Mutex );
    if ( InIndex >= Entries.size() )
    {
        return TUnexpected<FRHIError>( MakeRHIInvalidQuery( "Query index is out of range" ) );
    }
    FEntry &Entry = Entries[InIndex];
    if ( not Entry.bActive )
    {
        return TUnexpected<FRHIError>(
            FRHIError( ERHIErrorCode::InvalidState, ERHIErrorCondition::Query, "Query is not active" ) );
    }
    const UInt32 ValueCount = GetRHIQueryValueCount( Desc.Type ).ValueOr( 0U );
    for ( UInt32 ValueIndex = 0U; ValueIndex < ValueCount; ++ValueIndex )
    {
        Entry.Values[ValueIndex] = NextValue + ValueIndex;
    }
    Entry.Availability = NextAvailability;
    Entry.bAvailable   = true;
    Entry.bActive      = false;
    ++NextValue;
    return {};
}

void Lumen::RHI::Tests::FNullQueryPool::SetNextValue ( const UInt64 InValue ) noexcept
{
    HAL::TLockGuard<HAL::FPlatformRecursiveSharedMutex> Lock( Mutex );
    NextValue = InValue;
}

void Lumen::RHI::Tests::FNullQueryPool::SetNextAvailability ( const FRHITimelineValue InValue ) noexcept
{
    HAL::TLockGuard<HAL::FPlatformRecursiveSharedMutex> Lock( Mutex );
    NextAvailability = InValue;
}

Lumen::RHI::FRHIExpected Lumen::RHI::Tests::FNullQueryPool::SetQueryValue ( const UInt32 InIndex,
                                                                            const UInt64 InValue,
                                                                            const FRHITimelineValue InAvailability ) noexcept
{
    HAL::TLockGuard<HAL::FPlatformRecursiveSharedMutex> Lock( Mutex );
    if ( InIndex >= Entries.size() or Entries[InIndex].bActive )
    {
        return TUnexpected<FRHIError>( MakeRHIInvalidQuery( "Cannot set an invalid or active query result" ) );
    }
    FEntry &Entry      = Entries[InIndex];
    Entry.Values[0]    = InValue;
    Entry.Availability = InAvailability;
    Entry.bAvailable   = true;
    return {};
}

Lumen::Bool Lumen::RHI::Tests::FNullQueryPool::IsActive ( const UInt32 InIndex ) const noexcept
{
    HAL::TLockGuard<HAL::FPlatformRecursiveSharedMutex> Lock( Mutex );
    return InIndex < Entries.size() and Entries[InIndex].bActive;
}

Lumen::TOptional<Lumen::RHI::FRHIError> Lumen::RHI::Tests::FNullQueryPool::CheckFault ( const FStringView InOperation ) noexcept
{
    return FaultPlan ? FaultPlan->ShouldFail( InOperation ) : TOptional<FRHIError>{};
}
