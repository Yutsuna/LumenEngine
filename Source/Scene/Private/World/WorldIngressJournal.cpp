/**
 * @file WorldIngressJournal.cpp
 * @brief Implementation of the bounded external ingress journal.
 */

#include <World/WorldIngressJournal.hpp>

#include <HAL/LockGuard.hpp>
#include <Templates/Unexpected.hpp>

Lumen::FWorldIngressJournal::FWorldIngressJournal () noexcept
{
    Reserve( DefaultCapacity );
}

void Lumen::FWorldIngressJournal::Reserve ( const USize InCapacity )
{
    HAL::TUniqueLock<HAL::FPlatformMutex> Lock( Mutex );
    Records.reserve( InCapacity );
    Capacity = InCapacity;
}

Lumen::TExpected<void, Lumen::EWorldIngressError> Lumen::FWorldIngressJournal::AppendMessage (
    const UInt64 InTargetFrame, const ActorID InTarget, const ActorID InSender, const FMessage &InMessage ) noexcept
{
    FWorldIngressRecord Record;
    Record.Kind        = EWorldIngressKind::Message;
    Record.Target      = InTarget;
    Record.Sender      = InSender;
    Record.Message     = InMessage;
    Record.TargetFrame = InTargetFrame;
    return Append( Record );
}

Lumen::TExpected<void, Lumen::EWorldIngressError> Lumen::FWorldIngressJournal::AppendBroadcast (
    const UInt64 InTargetFrame, const ActorID InSender, const FMessage &InMessage ) noexcept
{
    FWorldIngressRecord Record;
    Record.Kind        = EWorldIngressKind::Broadcast;
    Record.Target      = InvalidActorID;
    Record.Sender      = InSender;
    Record.Message     = InMessage;
    Record.TargetFrame = InTargetFrame;
    return Append( Record );
}

Lumen::TExpected<void, Lumen::EWorldIngressError> Lumen::FWorldIngressJournal::AppendDestroy ( const UInt64 InTargetFrame,
                                                                                               const ActorID InTarget ) noexcept
{
    FWorldIngressRecord Record;
    Record.Kind        = EWorldIngressKind::Destroy;
    Record.Target      = InTarget;
    Record.Sender      = InvalidActorID;
    Record.TargetFrame = InTargetFrame;
    return Append( Record );
}

Lumen::USize Lumen::FWorldIngressJournal::GetRecordCount () const noexcept
{
    HAL::TUniqueLock<HAL::FPlatformMutex> Lock( Mutex );
    return Records.size();
}

Lumen::USize Lumen::FWorldIngressJournal::GetCapacity () const noexcept
{
    return Capacity;
}

Lumen::UInt64 Lumen::FWorldIngressJournal::GetNextSequence () const noexcept
{
    HAL::TUniqueLock<HAL::FPlatformMutex> Lock( Mutex );
    return NextSequence;
}

Lumen::UInt64 Lumen::FWorldIngressJournal::GetDroppedCount () const noexcept
{
    HAL::TUniqueLock<HAL::FPlatformMutex> Lock( Mutex );
    return DroppedCount;
}

Lumen::UInt64 Lumen::FWorldIngressJournal::GetGeneration () const noexcept
{
    HAL::TUniqueLock<HAL::FPlatformMutex> Lock( Mutex );
    return Generation;
}

Lumen::USize Lumen::FWorldIngressJournal::CopyRecords ( const TSpan<FWorldIngressRecord> OutRecords ) const noexcept
{
    HAL::TUniqueLock<HAL::FPlatformMutex> Lock( Mutex );
    const USize Count = OutRecords.size() < Records.size() ? OutRecords.size() : Records.size();
    for ( USize Index = 0ULL; Index < Count; ++Index )
    {
        OutRecords[Index] = Records[Index];
    }
    return Count;
}

Lumen::USize Lumen::FWorldIngressJournal::CopyReadyRecords ( const UInt64 InMaxFrame,
                                                             const USize InStartIndex,
                                                             const TSpan<FWorldIngressRecord> OutRecords ) const noexcept
{
    HAL::TUniqueLock<HAL::FPlatformMutex> Lock( Mutex );
    USize Copied = 0ULL;
    for ( USize Index = InStartIndex; Index < Records.size() and Copied < OutRecords.size(); ++Index )
    {
        if ( Records[Index].TargetFrame > InMaxFrame )
        {
            break;
        }
        OutRecords[Copied++] = Records[Index];
    }
    return Copied;
}

void Lumen::FWorldIngressJournal::Clear () noexcept
{
    HAL::TUniqueLock<HAL::FPlatformMutex> Lock( Mutex );
    Records.clear();
    NextSequence    = 1ULL;
    LastTargetFrame = 0ULL;
    ++Generation;
}

Lumen::TExpected<void, Lumen::EWorldIngressError>
Lumen::FWorldIngressJournal::Append ( const FWorldIngressRecord &InRecord ) noexcept
{
    HAL::TUniqueLock<HAL::FPlatformMutex> Lock( Mutex );
    if ( InRecord.TargetFrame < LastTargetFrame )
    {
        return MakeUnexpected( EWorldIngressError::FrameOrderViolation );
    }
    if ( Records.size() >= Capacity )
    {
        ++DroppedCount;
        return MakeUnexpected( EWorldIngressError::CapacityExceeded );
    }

    FWorldIngressRecord Record = InRecord;
    Record.Sequence            = NextSequence++;
    LastTargetFrame            = Record.TargetFrame;
    Records.push_back( Record );
    return {};
}
